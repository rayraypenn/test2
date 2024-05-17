#include "ns3/grader-logs.h"
#include "ns3/ipv4-address.h"
#include <sstream>
#include <vector>

using namespace ns3;

void GraderLogs::HelloGrader(std::string currNodeID, Ipv4Address currNodeIP)
{
    PRINT_LOG("Hello grader! from node: " << currNodeID << "(" << currNodeIP << ")");
}

void GraderLogs::RingState(Ipv4Address currNodeIP,
                           std::string currNodeId,
                           uint32_t currNodeKey,
                           Ipv4Address predNodeIP,
                           std::string predNodeId,
                           uint32_t predNodeKey,
                           Ipv4Address succNodeIP,
                           std::string succNodeId,
                           uint32_t succNodeKey)
{
    std::ostringstream oss;
    oss << "Ring State\n";
    oss << "\tCurr<Node " << currNodeId << ", " << currNodeIP << ", " << PennKeyHelper::KeyToHexString(currNodeKey) << ">\n";
    oss << "\tPred<Node " << predNodeId << ", " << predNodeIP << ", " << PennKeyHelper::KeyToHexString(predNodeKey) << ">\n";
    oss << "\tSucc<Node " << succNodeId << ", " << succNodeIP << ", " << PennKeyHelper::KeyToHexString(succNodeKey) << ">\n";
    PRINT_LOG(oss.str());
}

void GraderLogs::EndOfRingState()
{
    PRINT_LOG("End of Ring State\n");
}


std::string GraderLogs::GetLookupIssueLogStr(uint32_t currentNodeKey, uint32_t targetKey)
{
    std::stringstream ss;
    ss << "LookupIssue<" << PennKeyHelper::KeyToHexString(currentNodeKey) << ", "
       << PennKeyHelper::KeyToHexString(targetKey) << ">";

    return ss.str();
}


std::string GraderLogs::GetLookupForwardingLogStr(uint32_t currentNodeKey,
                                         std::string nextNodeID,
                                         uint32_t nextNodeKey,
                                         uint32_t targetKey)
{
    std::stringstream ss;
    ss << "LookupRequest<" << PennKeyHelper::KeyToHexString(currentNodeKey)
       << ">: NextHop<" << nextNodeID << ", "
       << PennKeyHelper::KeyToHexString(nextNodeKey) << ", "
       << PennKeyHelper::KeyToHexString(targetKey) << ">";
    return ss.str();
}


std::string GraderLogs::GetLookupResultLogStr(uint32_t currNodeKey,
                                     uint32_t targetKey,
                                     std::string originatorNodeID,
                                     uint32_t originatorNodeKey)
{
    std::stringstream ss;
    ss << "LookupResult<" << PennKeyHelper::KeyToHexString(currNodeKey) << ", "
       << PennKeyHelper::KeyToHexString(targetKey) << ", "
       << originatorNodeID << ", "
       << PennKeyHelper::KeyToHexString(originatorNodeKey) << ">";
    return ss.str();
}


void GraderLogs::AverageHopCount(std::string currNodeId, uint16_t lookupCount, uint16_t lookupHopCount)
{
    PRINT_LOG("AvgHopCount<" << currNodeId << ", " << lookupCount << ", " << lookupHopCount << ">");
}

// ---------------  SEARCH logs ---------

std::string GraderLogs::GetPublishLogStr(std::string keyword, std::string docID)
{
    std::stringstream ss;
    ss << "Publish<" << keyword << ", " << docID << ">";
    return ss.str();
}


std::string GraderLogs::GetStoreLogStr(std::string keyword, std::string docID)
{
    std::stringstream ss;
    ss << "Store<" << keyword << ", " << docID << ">";
    return ss.str();
}


std::string GraderLogs::GetSearchLogStr(std::vector<std::string> terms)
{
    std::ostringstream ss;
    ss << "Search<";
    for (size_t i = 0; i < terms.size(); i++)
    {
        ss << terms[i] << (i == terms.size() - 1 ? ">" : ", ");
    }
    return ss.str();
}


std::string GraderLogs::GetInvertedListShipLogStr(std::string targetKeyword,
                                  std::vector<std::string> docs)
{
    std::stringstream ss;
    ss << "InvertedListShip<";
    ss << targetKeyword << ", ";
    if (docs.empty())
    {
        ss << "'Empty List'>";
    }
    else
    {
        ss << "{";
        size_t idx = 0;
        for (const auto &doc : docs)
        {
            ss << doc << (idx == docs.size() - 1 ? "}>" : ", ");
            idx += 1;
        }
    }
    return ss.str();
}


std::string GraderLogs::GetSearchResultsLogStr(Ipv4Address originatorNodeIP,
                               std::vector<std::string> docs)
{
    std::stringstream ss;
    ss << "SearchResults<";
    ss << originatorNodeIP << ", ";

    if (!docs.empty())
    {
        ss << "{";
        size_t idx = 0;
        for (const auto &doc : docs)
        {
            ss << doc << (idx == docs.size() - 1 ? "}>" : ", ");
            idx += 1;
        }
    }
    else
    {
        ss << "'Empty List'>";
    }
    return ss.str();
}
