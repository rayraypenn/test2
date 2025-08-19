/*
 * ripv2d - A minimal yet functional RIPv2 daemon (Linux)
 *
 * Features:
 *  - RIPv2 request/response over UDP/520, 224.0.0.9 multicast
 *  - Periodic and triggered updates, split horizon with poison reverse
 *  - Route timers: update (~30s jittered), invalid (180s), garbage (120s)
 *  - Simple password authentication (optional)
 *  - Kernel route install/delete via rtnetlink (protocol RTPROT_RIP)
 *
 * Limitations:
 *  - Linux-only (rtnetlink)
 *  - Interface list captured at startup; no dynamic link change handling
 *  - No MD5 authentication yet
 *  - Redistributes only connected routes on active interfaces
 *
 * Build: g++ -std=gnu++17 -O2 ripv2d.cpp -o ripv2d
 * Run: sudo ./ripv2d [-c /etc/ripv2d.conf]
 */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <cstdio>
#include <map>
#include <optional>
#include <poll.h>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <ifaddrs.h>
#include <algorithm>
#include <random>
#include <ctime>
#include <cctype>

using namespace std::chrono;

static const uint16_t RIP_PORT = 520;
static const uint32_t RIP_MCAST = 0xE0000009u; // 224.0.0.9

static const uint8_t RIP_CMD_REQUEST = 1;
static const uint8_t RIP_CMD_RESPONSE = 2;
static const uint16_t RIP_V2 = 2;
static const uint16_t RIP_AUTH_AFI = 0xFFFF;
static const uint16_t RIP_AF_INET = 2;
static const uint32_t RIP_METRIC_INFINITY = 16;

static const seconds TIMER_UPDATE_MIN(25); // jittered 25-35s
static const seconds TIMER_UPDATE_MAX(35);
static const seconds TIMER_INVALID(180);
static const seconds TIMER_GARBAGE(120);
static const seconds TIMER_TRIGGER_MIN(1);
static const seconds TIMER_TRIGGER_MAX(5);

// Logging
enum class LogLevel { DEBUG=0, INFO=1, WARN=2, ERROR=3 };
static LogLevel g_log_level = LogLevel::INFO;

#define LOG(level, msg) do { \
  if ((int)(level) >= (int)g_log_level) { \
    auto now = system_clock::now(); \
    auto t = system_clock::to_time_t(now); \
    char tb[32]; \
    std::strftime(tb, sizeof(tb), "%F %T", std::localtime(&t)); \
    std::cerr << "[" << tb << "] "; \
    switch(level){case LogLevel::DEBUG: std::cerr<< "DEBUG";break;case LogLevel::INFO: std::cerr<< "INFO";break;case LogLevel::WARN: std::cerr<< "WARN";break;case LogLevel::ERROR: std::cerr<< "ERROR";break;} \
    std::cerr << " ripv2d: " << msg << std::endl; \
  } \
} while(0)

static uint64_t now_ms() {
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// Random jitter helpers
static std::mt19937_64 rng{std::random_device{}()};
static seconds rand_between(seconds a, seconds b) {
  std::uniform_int_distribution<long long> dist(a.count(), b.count());
  return seconds(dist(rng));
}

// Configuration
struct Config {
  std::unordered_set<std::string> interfaces_allow;
  std::unordered_set<std::string> passive_ifaces;
  std::string password; // simple auth (max 16)
  LogLevel level = LogLevel::INFO;
};

static std::string trim(const std::string& s) {
  size_t i=0, j=s.size();
  while (i<j && std::isspace((unsigned char)s[i])) ++i;
  while (j>i && std::isspace((unsigned char)s[j-1])) --j;
  return s.substr(i, j-i);
}

static void split_csv(const std::string& v, std::unordered_set<std::string>& out) {
  std::stringstream ss(v);
  std::string token;
  while (std::getline(ss, token, ',')) {
    token = trim(token);
    if (!token.empty()) out.insert(token);
  }
}

static bool load_config(const std::string& path, Config& cfg) {
  FILE* f = fopen(path.c_str(), "r");
  if (!f) {
    LOG(LogLevel::WARN, "Config not found: " << path << " (using defaults)");
    return false;
  }
  char line[1024];
  while (fgets(line, sizeof(line), f)) {
    std::string s(line);
    s = trim(s);
    if (s.empty() || s[0]=='#') continue;
    auto eq = s.find('=');
    if (eq==std::string::npos) continue;
    std::string key = trim(s.substr(0, eq));
    std::string val = trim(s.substr(eq+1));
    if (key=="interfaces") {
      split_csv(val, cfg.interfaces_allow);
    } else if (key=="passive_interfaces") {
      split_csv(val, cfg.passive_ifaces);
    } else if (key=="password") {
      cfg.password = val;
      if (cfg.password.size() > 16) cfg.password.resize(16);
    } else if (key=="log_level") {
      std::string v = val;
      std::transform(v.begin(), v.end(), v.begin(), ::tolower);
      if (v=="debug") cfg.level = LogLevel::DEBUG;
      else if (v=="info") cfg.level = LogLevel::INFO;
      else if (v=="warn") cfg.level = LogLevel::WARN;
      else if (v=="error") cfg.level = LogLevel::ERROR;
    }
  }
  fclose(f);
  return true;
}

// Network helpers
static uint32_t ip4_to_u32(const in_addr& a){ return ntohl(a.s_addr); }
static in_addr u32_to_ip4(uint32_t x){ in_addr a; a.s_addr = htonl(x); return a; }
static std::string ip4_to_str(uint32_t x){
  char b[INET_ADDRSTRLEN];
  in_addr a = u32_to_ip4(x);
  inet_ntop(AF_INET, &a, b, sizeof(b));
  return b;
}
static uint32_t mask_len(uint32_t mask) {
  return __builtin_popcount(mask);
}
static uint32_t mask_from_len(uint32_t len) {
  if (len==0) return 0;
  return htonl(~((1u << (32-len)) - 1));
}

// RIPv2 wire structures
#pragma pack(push,1)
struct RipHeader {
  uint8_t command;
  uint8_t version;
  uint16_t zero;
};

struct RipRte {
  uint16_t afi;
  uint16_t route_tag;
  uint32_t ip;
  uint32_t mask;
  uint32_t nexthop;
  uint32_t metric; // 1..16 (network order on wire)
};
#pragma pack(pop)

// Interface representation
struct Iface {
  std::string name;
  unsigned ifindex = 0;
  uint32_t addr = 0;   // IPv4 address (host order)
  uint32_t mask = 0;   // netmask (host order)
  bool passive = false;
};

struct RouteEntry {
  uint32_t prefix = 0;
  uint32_t mask = 0;
  uint32_t nexthop = 0; // next hop IP (host order)
  unsigned oif = 0;     // outgoing interface index
  std::string oif_name;
  uint32_t metric = RIP_METRIC_INFINITY;
  uint16_t route_tag = 0;

  // Timers
  uint64_t changed_at_ms = 0; // for triggered update throttle
  uint64_t updated_ms = 0;    // last fresh update (for invalidation)
  bool valid = true;
  bool garbage = false;

  // Learned-from neighbor (sender IP)
  uint32_t learned_from = 0;
};

// Route table keyed by (prefix,mask)
struct PrefixKey {
  uint32_t p;
  uint32_t m;
  bool operator==(const PrefixKey& o) const { return p==o.p && m==o.m; }
};
struct PrefixKeyHash {
  size_t operator()(const PrefixKey& k) const {
    return std::hash<uint64_t>{}((uint64_t)k.p << 32 | k.m);
  }
};

class Netlink {
  int fd = -1;
public:
  Netlink() {
    fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
      perror("netlink socket");
      throw std::runtime_error("netlink socket failed");
    }
  }
  ~Netlink(){ if (fd>=0) close(fd); }

  bool add_replace_route(uint32_t dst, uint32_t mask, uint32_t gw, unsigned oif, uint32_t metric) {
    // Add or replace an IPv4 route
    struct {
      nlmsghdr nlh;
      rtmsg rtm;
      char attrbuf[256];
    } req {};
    req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg));
    req.nlh.nlmsg_type = RTM_NEWROUTE;
    req.nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;
    req.nlh.nlmsg_seq = 1;
    req.rtm.rtm_family = AF_INET;
    req.rtm.rtm_dst_len = mask_len(mask);
    req.rtm.rtm_table = RT_TABLE_MAIN;
    req.rtm.rtm_protocol = RTPROT_RIP;
    req.rtm.rtm_scope = RT_SCOPE_UNIVERSE;
    req.rtm.rtm_type = (metric >= RIP_METRIC_INFINITY) ? RTN_UNREACHABLE : RTN_UNICAST;

    auto addattr = [&](uint16_t type, const void* data, size_t len){
      struct rtattr* rta = (struct rtattr*)((char*)&req + req.nlh.nlmsg_len);
      rta->rta_type = type;
      rta->rta_len = RTA_LENGTH(len);
      memcpy(RTA_DATA(rta), data, len);
      req.nlh.nlmsg_len = NLMSG_ALIGN(req.nlh.nlmsg_len) + RTA_LENGTH(len);
    };

    uint32_t dst_be = htonl(dst);
    if (req.rtm.rtm_dst_len > 0) addattr(RTA_DST, &dst_be, 4);
    if (gw != 0) {
      uint32_t gw_be = htonl(gw);
      addattr(RTA_GATEWAY, &gw_be, 4);
    }
    if (oif != 0) addattr(RTA_OIF, &oif, 4);
    // Set a priority derived from metric to bias kernel ECMP decisions
    addattr(RTA_PRIORITY, &metric, sizeof(metric));

    if (send(fd, &req, req.nlh.nlmsg_len, 0) < 0) {
      LOG(LogLevel::ERROR, "rtnetlink add/replace failed: " << strerror(errno));
      return false;
    }
    return true;
  }

  bool delete_route(uint32_t dst, uint32_t mask, uint32_t gw, unsigned oif) {
    struct {
      nlmsghdr nlh;
      rtmsg rtm;
      char attrbuf[256];
    } req {};
    req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(rtmsg));
    req.nlh.nlmsg_type = RTM_DELROUTE;
    req.nlh.nlmsg_flags = NLM_F_REQUEST;
    req.nlh.nlmsg_seq = 2;
    req.rtm.rtm_family = AF_INET;
    req.rtm.rtm_dst_len = mask_len(mask);
    req.rtm.rtm_table = RT_TABLE_MAIN;

    auto addattr = [&](uint16_t type, const void* data, size_t len){
      struct rtattr* rta = (struct rtattr*)((char*)&req + req.nlh.nlmsg_len);
      rta->rta_type = type;
      rta->rta_len = RTA_LENGTH(len);
      memcpy(RTA_DATA(rta), data, len);
      req.nlh.nlmsg_len = NLMSG_ALIGN(req.nlh.nlmsg_len) + RTA_LENGTH(len);
    };
    uint32_t dst_be = htonl(dst);
    if (req.rtm.rtm_dst_len > 0) addattr(RTA_DST, &dst_be, 4);
    if (gw != 0) { uint32_t gw_be = htonl(gw); addattr(RTA_GATEWAY, &gw_be, 4); }
    if (oif != 0) addattr(RTA_OIF, &oif, 4);

    if (send(fd, &req, req.nlh.nlmsg_len, 0) < 0) {
      LOG(LogLevel::ERROR, "rtnetlink delete failed: " << strerror(errno));
      return false;
    }
    return true;
  }
};

// Global state
struct Daemon {
  Config cfg;
  int sock = -1;
  std::vector<Iface> ifaces;
  std::unordered_map<PrefixKey, RouteEntry, PrefixKeyHash> table;
  Netlink nl;
  // timers
  uint64_t next_periodic_ms = 0;
  uint64_t next_trigger_ms = 0;
  bool trigger_scheduled = false;

  Daemon(const Config& c) : cfg(c), nl() {}

  ~Daemon(){ if (sock>=0) close(sock); }

  bool init_socket() {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return false; }
    int one = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    // Enable IP_PKTINFO to learn incoming interface
    setsockopt(sock, IPPROTO_IP, IP_PKTINFO, &one, sizeof(one));
    // Bind to UDP/520 on INADDR_ANY
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(RIP_PORT);
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
      perror("bind UDP/520");
      return false;
    }
    // Join multicast group on each iface later
    return true;
  }

  bool enumerate_ifaces() {
    ifaddrs* ifa = nullptr;
    if (getifaddrs(&ifa) != 0) { perror("getifaddrs"); return false; }
    std::unordered_map<std::string, Iface> tmp;
    for (ifaddrs* p=ifa; p; p=p->ifa_next) {
      if (!p->ifa_addr) continue;
      if (p->ifa_addr->sa_family != AF_INET) continue;
      std::string name = p->ifa_name ? p->ifa_name : "";
      if (name.empty()) continue;
      // skip loopback
      if (p->ifa_flags & IFF_LOOPBACK) continue;
      // must be up
      if (!(p->ifa_flags & IFF_UP)) continue;

      if (!cfg.interfaces_allow.empty() && !cfg.interfaces_allow.count(name)) continue;

      Iface& iface = tmp[name];
      iface.name = name;
      iface.ifindex = if_nametoindex(name.c_str());
      auto* sin = (sockaddr_in*)p->ifa_addr;
      auto* nm = (sockaddr_in*)p->ifa_netmask;
      iface.addr = ntohl(sin->sin_addr.s_addr);
      iface.mask = ntohl(nm->sin_addr.s_addr);
      iface.passive = cfg.passive_ifaces.count(name) > 0;
    }
    freeifaddrs(ifa);
    ifaces.clear();
    for (auto& kv : tmp) ifaces.push_back(kv.second);
    if (ifaces.empty()) {
      LOG(LogLevel::ERROR, "No eligible interfaces found");
      return false;
    }
    LOG(LogLevel::INFO, "Using " << ifaces.size() << " interface(s)");
    for (auto& i : ifaces) {
      LOG(LogLevel::INFO, "  iface=" << i.name << " addr=" << ip4_to_str(i.addr)
          << "/" << mask_len(i.mask) << (i.passive? " passive":""));
    }
    return true;
  }

  bool join_multicast() {
    for (auto& i : ifaces) {
      ip_mreqn mreq{};
      mreq.imr_multiaddr.s_addr = htonl(RIP_MCAST);
      mreq.imr_address.s_addr = htonl(i.addr);
      mreq.imr_ifindex = i.ifindex;
      if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        // Fallback without imr_ifindex
        ip_mreq mr{};
        mr.imr_multiaddr.s_addr = htonl(RIP_MCAST);
        mr.imr_interface.s_addr = htonl(i.addr);
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
          LOG(LogLevel::WARN, "Join multicast failed on " << i.name << ": " << strerror(errno));
        }
      }
    }
    // Set multicast TTL to 1
    uint8_t ttl = 1;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    // Enable loopback so we can see our own packets (optional)
    uint8_t loop = 1;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
    return true;
  }

  void originate_connected_routes() {
    uint64_t t = now_ms();
    for (const auto& i : ifaces) {
      // Connected network prefix
      uint32_t prefix = i.addr & i.mask;
      PrefixKey k{prefix, i.mask};
      // Skip duplicates
      if (table.find(k) != table.end()) continue;
      RouteEntry re;
      re.prefix = prefix;
      re.mask = i.mask;
      re.nexthop = 0;
      re.metric = 1;
      re.oif = i.ifindex;
      re.oif_name = i.name;
      re.updated_ms = t;
      re.changed_at_ms = t;
      re.valid = true;
      re.garbage = false;
      re.learned_from = 0; // self-originated
      table[k] = re;
      LOG(LogLevel::INFO, "Originate connected " << ip4_to_str(prefix) << "/" << mask_len(i.mask) << " via " << i.name);
      // Do not install connected routes (assumed present in kernel)
    }
  }

  void schedule_periodic() {
    next_periodic_ms = now_ms() + duration_cast<milliseconds>(rand_between(TIMER_UPDATE_MIN, TIMER_UPDATE_MAX)).count();
  }
  void schedule_triggered() {
    if (trigger_scheduled) return;
    trigger_scheduled = true;
    next_trigger_ms = now_ms() + duration_cast<milliseconds>(rand_between(TIMER_TRIGGER_MIN, TIMER_TRIGGER_MAX)).count();
  }

  void send_response_on_iface(const Iface& iface, const sockaddr_in* unicast = nullptr) {
    if (iface.passive && unicast==nullptr) {
      // Passive: do not send periodic multicast updates
      return;
    }
    // Destination
    sockaddr_in dst{};
    if (unicast) {
      dst = *unicast;
    } else {
      dst.sin_family = AF_INET;
      dst.sin_port = htons(RIP_PORT);
      dst.sin_addr.s_addr = htonl(RIP_MCAST);
    }
    // Set outbound interface for multicast
    in_addr out_if{};
    out_if.s_addr = htonl(iface.addr);
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &out_if, sizeof(out_if)) < 0) {
      LOG(LogLevel::WARN, "IP_MULTICAST_IF " << iface.name << " failed: " << strerror(errno));
    }

    // Build response packets up to 25 RTEs each
    std::vector<uint8_t> pkt;
    auto push = [&](const void* data, size_t len){ size_t o=pkt.size(); pkt.resize(o+len); memcpy(pkt.data()+o, data, len); };

    RipHeader hdr{RIP_CMD_RESPONSE, RIP_V2, 0};
    // Prepare list of routes to advertise
    std::vector<RouteEntry> routes;
    routes.reserve(table.size());
    for (const auto& kv : table) {
      const RouteEntry& re = kv.second;
      routes.push_back(re);
    }
    // Chunking
    size_t idx = 0;
    while (idx < routes.size()) {
      pkt.clear();
      push(&hdr, sizeof(hdr));
      size_t count = 0;
      const size_t max_rtes = cfg.password.empty() ? 25 : 24;
      for (; idx < routes.size() && count < max_rtes; ++idx, ++count) {
        const RouteEntry& re = routes[idx];
        RipRte rte{};
        rte.afi = htons(RIP_AF_INET);
        rte.route_tag = htons(re.route_tag);
        rte.ip = htonl(re.prefix);
        rte.mask = htonl(re.mask);
        rte.nexthop = htonl(0); // we typically set 0; receivers use sender or provided next hop
        uint32_t metric = re.metric;
        // Poison reverse: if the route was learned via this iface, advertise metric 16
        if (!re.oif_name.empty() && re.oif == iface.ifindex && re.learned_from != 0) {
          metric = RIP_METRIC_INFINITY;
        }
        rte.metric = htonl(metric);
        push(&rte, sizeof(rte));
      }
      // Authentication: If configured, prepend an auth RTE at the beginning
      if (!cfg.password.empty()) {
        RipRte auth{};
        auth.afi = htons(RIP_AUTH_AFI);
        auth.route_tag = htons(2); // simple password
        // 16-byte password goes in bytes after route_tag
        uint8_t authbuf[16] = {0};
        memcpy(authbuf, cfg.password.data(), std::min(cfg.password.size(), sizeof(authbuf)));
        // Grow packet and shift existing RTEs
        pkt.resize(pkt.size() + sizeof(RipRte));
        memmove(pkt.data()+sizeof(hdr)+sizeof(RipRte), pkt.data()+sizeof(hdr), pkt.size()-sizeof(hdr)-sizeof(RipRte));
        memcpy(pkt.data()+sizeof(hdr), &auth, sizeof(auth));
        // Copy password starting at offset 4 within the auth RTE (over ip..metric fields)
        memcpy(pkt.data()+sizeof(hdr)+4, authbuf, 16);
      }

      ssize_t n = sendto(sock, pkt.data(), pkt.size(), 0, (sockaddr*)&dst, sizeof(dst));
      if (n < 0) {
        LOG(LogLevel::WARN, "sendto on " << iface.name << " failed: " << strerror(errno));
      } else {
        LOG(LogLevel::DEBUG, "Sent RIPv2 response on " << iface.name << " (" << n << " bytes)");
      }
    }
  }

  void send_full_table() {
    for (const auto& iface : ifaces) {
      send_response_on_iface(iface);
    }
  }

  void handle_request(const uint8_t* buf, size_t len, const Iface& rx_if, const sockaddr_in& src) {
    // If it's a full table request (first RTE has AFI=0 and metric=16), send full table to src
    if (len < sizeof(RipHeader) + sizeof(RipRte)) return;
    const RipRte* rte = reinterpret_cast<const RipRte*>(buf + sizeof(RipHeader));
    if (ntohs(rte->afi) == 0 && ntohl(rte->metric) == RIP_METRIC_INFINITY) {
      // Unicast full table to requester
      sockaddr_in dst = src;
      send_response_on_iface(rx_if, &dst);
      LOG(LogLevel::DEBUG, "Responded to full table request from " << ip4_to_str(ntohl(src.sin_addr.s_addr)) << " on " << rx_if.name);
    } else {
      // Specific entries request: reply with matching metrics (simplified: send full table anyway)
      sockaddr_in dst = src;
      send_response_on_iface(rx_if, &dst);
      LOG(LogLevel::DEBUG, "Responded to specific request from " << ip4_to_str(ntohl(src.sin_addr.s_addr)) << " on " << rx_if.name);
    }
  }

  bool route_is_connected(uint32_t prefix, uint32_t mask) const {
    for (const auto& i : ifaces) {
      if ( (i.addr & i.mask) == prefix && i.mask == mask) return true;
    }
    return false;
  }

  void install_or_withdraw_kernel(const RouteEntry& re) {
    if (route_is_connected(re.prefix, re.mask)) {
      // Skip kernel ops for connected
      return;
    }
    if (re.metric < RIP_METRIC_INFINITY && re.nexthop != 0 && re.oif != 0) {
      nl.add_replace_route(re.prefix, re.mask, re.nexthop, re.oif, re.metric);
    } else {
      nl.delete_route(re.prefix, re.mask, re.nexthop, re.oif);
    }
  }

  void handle_response(const uint8_t* buf, size_t len, const Iface& rx_if, uint32_t src_ip) {
    size_t off = sizeof(RipHeader);
    // Authentication check if present
    if (len >= off + sizeof(RipRte)) {
      const RipRte* first = reinterpret_cast<const RipRte*>(buf + off);
      if (ntohs(first->afi) == RIP_AUTH_AFI) {
        uint16_t type = ntohs(first->route_tag);
        if (type == 2) {
          // simple password: 16 bytes start at ip field
          if (!cfg.password.empty()) {
            char pw[17]; memset(pw, 0, sizeof(pw));
            memcpy(pw, buf + off + 4, 16);
            std::string got(pw, strnlen(pw, 16));
            if (got != cfg.password) {
              LOG(LogLevel::WARN, "Auth failed from " << ip4_to_str(src_ip) << " on " << rx_if.name);
              return;
            }
          }
        } else {
          LOG(LogLevel::WARN, "Unsupported auth type from " << ip4_to_str(src_ip));
          return;
        }
        off += sizeof(RipRte);
      } else {
        // If we require password but none present, drop
        if (!cfg.password.empty()) {
          LOG(LogLevel::WARN, "Missing auth from " << ip4_to_str(src_ip));
          return;
        }
      }
    }

    bool any_change = false;
    for (; off + sizeof(RipRte) <= len; off += sizeof(RipRte)) {
      const RipRte* rte = reinterpret_cast<const RipRte*>(buf + off);
      if (ntohs(rte->afi) != RIP_AF_INET) continue;
      uint32_t ip = ntohl(rte->ip);
      uint32_t mask = ntohl(rte->mask);
      uint32_t nh = ntohl(rte->nexthop);
      uint32_t m = ntohl(rte->metric);
      if (m < 1 || m > RIP_METRIC_INFINITY) continue;

      // Compute next hop: use provided if on-link, else sender
      uint32_t chosen_nh = src_ip;
      if (nh != 0) {
        // on-link if (nh & rx_if.mask) == (rx_if.addr & rx_if.mask)
        if ( (nh & rx_if.mask) == (rx_if.addr & rx_if.mask) ) {
          chosen_nh = nh;
        }
      }
      // Normalize prefix
      uint32_t prefix = ip & mask;
      // Allow default route (mask could be 0)
      uint32_t new_metric = std::min(RIP_METRIC_INFINITY, m + 1);

      // Ignore routes to connected networks (we originate those)
      if (route_is_connected(prefix, mask)) continue;

      PrefixKey key{prefix, mask};
      uint64_t t = now_ms();
      auto it = table.find(key);
      if (it == table.end()) {
        // New route
        RouteEntry re;
        re.prefix = prefix;
        re.mask = mask;
        re.nexthop = chosen_nh;
        re.metric = new_metric;
        re.oif = rx_if.ifindex;
        re.oif_name = rx_if.name;
        re.route_tag = ntohs(rte->route_tag);
        re.updated_ms = t;
        re.changed_at_ms = t;
        re.valid = new_metric < RIP_METRIC_INFINITY;
        re.garbage = false;
        re.learned_from = src_ip;
        table[key] = re;
        install_or_withdraw_kernel(re);
        any_change = true;
        LOG(LogLevel::INFO, "Learned " << ip4_to_str(prefix) << "/" << mask_len(mask)
            << " via " << ip4_to_str(chosen_nh) << " metric " << new_metric << " on " << rx_if.name);
      } else {
        RouteEntry& re = it->second;
        bool from_same = (re.learned_from == src_ip);
        if (from_same) {
          // Update from same neighbor
          if (new_metric != re.metric || chosen_nh != re.nexthop) {
            re.metric = new_metric;
            re.nexthop = chosen_nh;
            re.oif = rx_if.ifindex;
            re.oif_name = rx_if.name;
            re.changed_at_ms = t;
            install_or_withdraw_kernel(re);
            any_change = true;
            LOG(LogLevel::INFO, "Updated " << ip4_to_str(prefix) << "/" << mask_len(mask)
                << " metric=" << re.metric);
          }
          re.updated_ms = t;
          re.valid = new_metric < RIP_METRIC_INFINITY;
          if (re.metric >= RIP_METRIC_INFINITY) {
            re.garbage = true;
          } else {
            re.garbage = false;
          }
        } else {
          // Different neighbor; consider better route
          if (new_metric < re.metric) {
            re.metric = new_metric;
            re.nexthop = chosen_nh;
            re.oif = rx_if.ifindex;
            re.oif_name = rx_if.name;
            re.learned_from = src_ip;
            re.updated_ms = t;
            re.changed_at_ms = t;
            re.valid = new_metric < RIP_METRIC_INFINITY;
            re.garbage = false;
            install_or_withdraw_kernel(re);
            any_change = true;
            LOG(LogLevel::INFO, "Selected better " << ip4_to_str(prefix) << "/" << mask_len(mask)
                << " via " << ip4_to_str(chosen_nh) << " metric " << new_metric);
          }
        }
      }
    }
    if (any_change) schedule_triggered();
  }

  std::optional<Iface> iface_by_index(unsigned idx) const {
    for (const auto& i : ifaces) if (i.ifindex == idx) return i;
    return std::nullopt;
  }

  void handle_incoming() {
    // Receive with control messages to get in_pktinfo (interface)
    uint8_t buf[1500];
    iovec iov{buf, sizeof(buf)};
    sockaddr_in src{};
    msghdr msg{};
    msg.msg_name = &src;
    msg.msg_namelen = sizeof(src);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    char cmsgbuf[CMSG_SPACE(sizeof(in_pktinfo))];
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    ssize_t n = recvmsg(sock, &msg, 0);
    if (n <= 0) {
      if (errno==EINTR) return;
      LOG(LogLevel::WARN, "recvmsg failed: " << strerror(errno));
      return;
    }
    unsigned ifidx = 0;
    for (cmsghdr* c = CMSG_FIRSTHDR(&msg); c; c = CMSG_NXTHDR(&msg, c)) {
      if (c->cmsg_level == IPPROTO_IP && c->cmsg_type == IP_PKTINFO) {
        in_pktinfo* pi = (in_pktinfo*)CMSG_DATA(c);
        ifidx = pi->ipi_ifindex;
      }
    }
    auto rx_if_opt = iface_by_index(ifidx);
    if (!rx_if_opt) {
      LOG(LogLevel::DEBUG, "Packet on unknown iface idx " << ifidx);
      return;
    }
    Iface rx_if = *rx_if_opt;

    if (n < (ssize_t)sizeof(RipHeader)) return;
    RipHeader* hdr = (RipHeader*)buf;
    if (hdr->version != RIP_V2) return;
    uint32_t src_ip = ntohl(src.sin_addr.s_addr);
    if (hdr->command == RIP_CMD_REQUEST) {
      handle_request(buf, n, rx_if, src);
    } else if (hdr->command == RIP_CMD_RESPONSE) {
      handle_response(buf, n, rx_if, src_ip);
    }
  }

  void age_routes_and_gc() {
    uint64_t t = now_ms();
    std::vector<PrefixKey> to_delete;
    for (auto& kv : table) {
      const PrefixKey& key = kv.first;
      RouteEntry& re = kv.second;
      // Skip connected
      if (route_is_connected(re.prefix, re.mask)) continue;

      // Invalid after TIMER_INVALID
      if (re.valid && t - re.updated_ms >= duration_cast<milliseconds>(TIMER_INVALID).count()) {
        re.valid = false;
        re.metric = RIP_METRIC_INFINITY;
        re.garbage = true;
        re.changed_at_ms = t;
        install_or_withdraw_kernel(re);
        schedule_triggered();
        LOG(LogLevel::INFO, "Route invalidated " << ip4_to_str(re.prefix) << "/" << mask_len(re.mask));
      }
      // Garbage collect after TIMER_INVALID + TIMER_GARBAGE
      if (re.garbage && t - re.updated_ms >= duration_cast<milliseconds>(TIMER_INVALID + TIMER_GARBAGE).count()) {
        to_delete.push_back(key);
      }
    }
    for (auto& k : to_delete) {
      auto it = table.find(k);
      if (it != table.end()) {
        RouteEntry& re = it->second;
        LOG(LogLevel::INFO, "Route removed " << ip4_to_str(re.prefix) << "/" << mask_len(re.mask));
        nl.delete_route(re.prefix, re.mask, re.nexthop, re.oif);
        table.erase(it);
      }
    }
  }

  void event_loop() {
    schedule_periodic();
    while (true) {
      int timeout_ms = 1000; // default 1s
      uint64_t t = now_ms();
      if (t >= next_periodic_ms) {
        // Send periodic updates
        for (const auto& i : ifaces) send_response_on_iface(i);
        schedule_periodic();
      }
      if (trigger_scheduled && t >= next_trigger_ms) {
        for (const auto& i : ifaces) send_response_on_iface(i);
        trigger_scheduled = false;
      }
      age_routes_and_gc();

      // Compute next deadline
      t = now_ms();
      int d1 = (int)std::max<int64_t>(0, (int64_t)next_periodic_ms - (int64_t)t);
      int d2 = trigger_scheduled ? (int)std::max<int64_t>(0, (int64_t)next_trigger_ms - (int64_t)t) : 1000;
      timeout_ms = std::min({timeout_ms, d1, d2});

      struct pollfd pfd{sock, POLLIN, 0};
      int r = poll(&pfd, 1, timeout_ms);
      if (r > 0 && (pfd.revents & POLLIN)) {
        handle_incoming();
      }
    }
  }
};

static void usage(const char* prog) {
  std::cerr << "Usage: " << prog << " [-c /etc/ripv2d.conf] [-v]" << std::endl;
}

int main(int argc, char** argv) {
  std::string conf = "/etc/ripv2d.conf";
  for (int i=1; i<argc; ++i) {
    std::string a = argv[i];
    if ((a=="-c" || a=="--config") && i+1<argc) {
      conf = argv[++i];
    } else if (a=="-v" || a=="--version") {
      std::cout << "ripv2d 0.1\n";
      return 0;
    } else if (a=="-h" || a=="--help") {
      usage(argv[0]); return 0;
    }
  }
  Config cfg;
  load_config(conf, cfg);
  g_log_level = cfg.level;

  try {
    Daemon d(cfg);
    if (!d.init_socket()) return 1;
    if (!d.enumerate_ifaces()) return 1;
    d.join_multicast();
    d.originate_connected_routes();
    LOG(LogLevel::INFO, "ripv2d started");
    d.event_loop();
  } catch (const std::exception& e) {
    LOG(LogLevel::ERROR, std::string("fatal: ") + e.what());
    return 1;
  }
  return 0;
}
