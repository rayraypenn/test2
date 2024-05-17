#ifndef GRADER_LOGS_H
#define GRADER_LOGS_H

#include "ns3/penn-application.h"
#include "ns3/ipv4-address.h"
#include "ns3/penn-key-helper.h"
#include <string>
#include <vector>

using namespace ns3;

class GraderLogs
{
public:
    // ---------------  CHORD logs ---------

    /**
     * @brief At any node X, a “X PENNSEARCH CHORD RINGSTATE” command will
     * initiate a ring output message that initiates from node X, and traverse
     * the entire Chord ring in a clockwise direction.  Each node node receiving
     * ring output message should call this method to log its state.
     */
    static void RingState(Ipv4Address currNodeIP,
                          std::string currNodeId,
                          uint32_t currNodeKey,
                          Ipv4Address predNodeIP,
                          std::string predNodeId,
                          uint32_t predNodeKey,
                          Ipv4Address succNodeIP,
                          std::string succNodeId,
                          uint32_t succNodeKey);

    static void EndOfRingState();

    /**
     * @brief Every time a node issues a lookup request use the returned string in CHORD_LOG
     *
     */
    static std::string GetLookupIssueLogStr(uint32_t currentNodeKey, uint32_t targetKey);

    /**
     * @brief Every time a node forwards a lookup request use returned string in CHORD_LOG
     *
     */
    static std::string GetLookupForwardingLogStr(uint32_t currentNodeKey,
                                 std::string nextNodeID,
                                 uint32_t nextNodeKey,
                                 uint32_t targetKey);

    /**
     * @brief  Every time a node returns a result in response to a lookup
     * request back to the node that originated the initial lookup request
     * use returned string in CHORD_LOG
     *
     */
    static std::string GetLookupResultLogStr(uint32_t currentNodeKey,
                             uint32_t targetKey,
                             std::string originatorNodeID,
                             uint32_t originatorNodeKey);

    /**
     * @brief Lookup count represents the total number of lookups issued
     * to this node and lookup hop count represents the total number of hops
     * taken for all lookups. Only count lookups that are issued from
     * PennSearch using the lookup API (do not count internal PennChord lookups).
     * Each node shoulf call this method on its chord destructor.
     *
     */
    static void AverageHopCount(std::string currNodeId, uint16_t lookupCount, uint16_t lookupHopCount);

    // ---------------  SEARCH logs ---------

    /**
     * @brief Whenever a node publishes a new inverted list entry
     * use string in SEARCH_LOG
     */
    static std::string GetPublishLogStr(std::string keyword, std::string docID);

    /**
     * @brief Whenever a node (that the keyword is hashed to) receives a new
     * inverted list entry to be stored use the returned string in SEARCH_LOG
     *
     */
    static std::string GetStoreLogStr(std::string keyword, std::string docID);

    /**
     * @brief Whenever a node issues a search query with terms T1, T2,. . . ,Tn use the returned string in SEARCH_LOG
     *
     */
    static std::string GetSearchLogStr(std::vector<std::string> terms);

    /**
     * @brief For each inverted list <docIDList> being shipped in the process
     * of the search use the returned string in SEARCH_LOG
     *
     */
    static std::string GetInvertedListShipLogStr(std::string targetKeyword,
                                 std::vector<std::string> docIdList);

    /**
     * @brief At the end of intersecting all keywords (T1, T2, . . . , Tn),
     * output the final document list <docIDList> that is being sent back to
     * the initial query node use the returned string in SEARCH_LOG
     *
     */
    static std::string GetSearchResultsLogStr(Ipv4Address originatorNodeIP,
                              std::vector<std::string> docIdList);

    /**
     * @brief used for testing
     *
     */
    static void HelloGrader(std::string currNodeID, Ipv4Address currNodeIP);
};

#endif