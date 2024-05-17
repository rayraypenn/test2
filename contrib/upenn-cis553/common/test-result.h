/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef TESTRESULT_H_
#define TESTRESULT_H_

#include "ns3/ipv4-address.h"
#include <string.h>
#include <string>

using namespace ns3;

void openResultFile(std::string filename);

void closeResultFile();

void openGraderResultFile(std::string filename);

void closeGraderResultFile();

void startDumpNeighbor();

void endDumpNeighbor();

void startDumpRoute();

void endDumpRoute();

void startDumpTrafficTrace();

void endDumpTrafficTrace();

void checkNeighborTableEntry(uint32_t neighborNum, Ipv4Address neighborAddr, Ipv4Address ifAddr);

void checkNeighborTableEntry(std::string neighborNum, Ipv4Address neighborAddr, Ipv4Address ifAddr);

void checkRouteTableEntry(uint32_t dstNum, Ipv4Address dstAddr, uint32_t nextHopNum, Ipv4Address nextHopAddr,
    Ipv4Address ifAddr, uint32_t cost);

void checkRouteTableEntry(std::string dstNum, Ipv4Address dstAddr, uint32_t nextHopNum, Ipv4Address nextHopAddr,
    Ipv4Address ifAddr, uint32_t cost);

void checkTrafficTrace(std::string g_nodeId, std::string g_moduleName);

#endif /* TESTRESULT_H_ */