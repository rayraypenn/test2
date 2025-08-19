ripv2d - RIPv2 routing daemon (C++17, POSIX/Linux)

Overview
- Implements RIPv2 (RFC 2453) for IPv4:
  - UDP/520, multicast 224.0.0.9
  - Request/Response, periodic and triggered updates
  - Split horizon with poison reverse
  - Next-hop processing
  - Timers: periodic ~30s (jittered), invalid 180s, garbage 120s
  - Simple password authentication (RIPv2 simple auth)
  - Installs routes into the kernel via rtnetlink (Linux)
- Single binary: ripv2d
- Minimal config file: /etc/ripv2d.conf (see ripv2d.conf.example)

Status
- Linux-only due to rtnetlink usage
- RIPv2 simple password supported; MD5 auth not implemented in this initial version

Build
- Requirements: g++ (C++17), Linux headers
- Build:
  cd ripv2d
  make
- Optional install:
  sudo make install PREFIX=/usr/local

Run
- Needs privileges to bind UDP/520 and join multicast, and to program routes:
  sudo ./ripv2d -c /etc/ripv2d.conf
- Without a config file, defaults are used.

Configuration
- Default path: /etc/ripv2d.conf
- Example (ripv2d.conf.example):
  # Comma-separated interface allowlist. If empty, all non-loopback IPv4 UP interfaces are used.
  interfaces=eth0,eth1

  # Comma-separated passive interfaces: listen but do not send updates on these
  passive_interfaces=eth1

  # RIPv2 simple password (max 16 bytes). Empty disables auth.
  password=secret123

  # Log level: debug, info, warn, error
  log_level=info
- Notes:
  - If password is set, incoming responses must carry a matching simple password auth RTE.
  - For interfaces in passive_interfaces, connected networks are advertised to others, but no multicast updates are sent out that interface.
  - Connected IPv4 networks on active interfaces are originated with metric 1.

Systemd unit (optional)
- See ripv2d.service:
  sudo cp ripv2d.service /etc/systemd/system/
  sudo systemctl daemon-reload
  sudo systemctl enable --now ripv2d

Interop testing
- You can interoperate with FRR or Quagga:
  - Configure FRR ripd with:
    router rip
      network eth0
      version 2
      no auto-summary
      passive-interface eth1
      !
    key-chain ripkey
      key 1
        key-string secret123
  - Or run a Linux network namespace lab and verify route exchanges.

Security
- Simple password authentication is cleartext (not cryptographically secure). Prefer MD5 auth in hostile environments; can be added later.

Limitations and TODO
- MD5 authentication (RFC 4822) not implemented
- Interface change monitoring (link up/down) is polled at start only
- No route redistribution beyond connected networks
- No CLI shell; configuration via file and flags only
