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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-apps-module.h"

#include <vector>
#include <iostream>
#include <sstream>
#include <unordered_map>

// Network Topology, Cybertwin: v1.0
//          
//   Core Cloud with 2 hosts
//   Edge Cloud with 4 hosts
//   End Devices with 8 hosts
//
using namespace ns3;

typedef std::pair<std::string, std::string> IPaddrBase;

void buildCsmaNetwork(NodeContainer &, const std::vector<uint32_t> &, IPaddrBase &, const char*, const uint32_t);
void p2pConnectToNeighbors(NodeContainer &, const uint32_t, const std::vector<uint32_t> &, std::vector<IPaddrBase> &, const char* , const char* );
void testNodesConnectivity(Ptr<Node> srcNode, Ptr<Node> dstNode);
std::vector<Ipv4Address> getNodeIpv4List(Ptr<Node> node);

// Define label for each node in order to cite node in container
enum NodeLabel
{
  END_HOST1, END_HOST2, // LAN1
  END_HOST3, END_HOST4, // LAN2
  END_HOST5, END_HOST6, // LAN3
  END_HOST7, END_HOST8, // LAN4
  EDGE_SERVER1, EDGE_SERVER2, EDGE_ROUTER1, // Edge cloud1
  EDGE_SERVER3, EDGE_SERVER4, EDGE_ROUTER2, // Edge cloud2
  CORE_SERVER1, CORE_SERVER2, CORE_ROUTER1, CORE_ROUTER2,// Core cloud
  MAX_NODE_NUM,
};

//-------------------------------------------------------------
//    define end LAN topology informations                    -
//-------------------------------------------------------------
enum {
  END_LAN1, // 10.1.0.0/24
  END_LAN2, // 10.2.0.0/24
  END_LAN3, // 10.3.0.0/24
  END_LAN4  // 10.4.0.0/24
};

const std::vector<std::vector<uint32_t> > endLANNodeIDs = {
  {END_HOST1, END_HOST2, EDGE_ROUTER1},
  {END_HOST3, END_HOST4, EDGE_ROUTER1},
  {END_HOST5, END_HOST6, EDGE_ROUTER2},
  {END_HOST7, END_HOST8, EDGE_ROUTER2},
};

std::vector<IPaddrBase> endLANIPBases = {
  std::make_pair("10.1.0.0", "255.255.255.0"),
  std::make_pair("10.2.0.0", "255.255.255.0"),
  std::make_pair("10.3.0.0", "255.255.255.0"),
  std::make_pair("10.4.0.0", "255.255.255.0")
};

//-------------------------------------------------------------
//    define edge router1 neighborhoods information           -
//-------------------------------------------------------------
const std::vector<uint32_t> 
edgeRouter1Neighbors = {
  EDGE_SERVER1,
  EDGE_SERVER2,
};

std::vector<IPaddrBase> edgeRouter1NeighborIPBase = {
  std::make_pair("20.1.0.0", "255.255.255.0"),
  std::make_pair("20.2.0.0", "255.255.255.0"),
};

//-------------------------------------------------------------
//    define edge router2 neighborhoods information           -
//-------------------------------------------------------------
const std::vector<uint32_t> 
edgeRouter2Neighbors = {
  EDGE_SERVER3,
  EDGE_SERVER4,
};

std::vector<IPaddrBase> edgeRouter2NeighborIPBase = {
  std::make_pair("20.3.0.0", "255.255.0.0"),
  std::make_pair("20.4.0.0", "255.255.0.0"),
};

//-------------------------------------------------------------
//    define core router1 neighborhoods information           -
//-------------------------------------------------------------
std::vector<uint32_t>
coreRouter1Neighbors = {
  EDGE_ROUTER1,
  EDGE_ROUTER2,
  CORE_SERVER1,
};

std::vector<IPaddrBase> coreRouter1NeighborIPBase = {
  std::make_pair("30.1.0.0", "255.255.255.0"),
  std::make_pair("30.3.0.0", "255.255.255.0"),
  std::make_pair("40.1.0.0", "255.255.255.0"),
};

//-------------------------------------------------------------
//    define core router2 neighborhoods information           -
//-------------------------------------------------------------
const std::vector<uint32_t>
coreRouter2Neighbors = {
  EDGE_ROUTER1,
  EDGE_ROUTER2,
  CORE_SERVER2,
};

std::vector<IPaddrBase> coreRouter2NeighborIPBase = {
  std::make_pair("30.2.0.0", "255.255.255.0"),
  std::make_pair("30.4.0.0", "255.255.255.0"),
  std::make_pair("40.2.0.0", "255.255.255.0"),
};

NS_LOG_COMPONENT_DEFINE ("Cybertwinv1");

int 
main (int argc, char *argv[])
{
  // parse command line arguements
  //CommandLine cmd (__FILE__);
  //cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  //cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  //cmd.Parse (argc,argv);
  LogComponentEnable("Cybertwinv1", LOG_LEVEL_INFO);
  LogComponentEnable ("V4Ping", LOG_LEVEL_DEBUG);

  //-------------------------------------------------------------
  //                     Create Nodes                           -
  //-------------------------------------------------------------
  NS_LOG_INFO("-> Creating Nodes.");
  NodeContainer allNodesContainer;
  allNodesContainer.Create(MAX_NODE_NUM);

  //-------------------------------------------------------------
  //                     Install Network Stack                  -
  //-------------------------------------------------------------
  NS_LOG_INFO("-> Install Network Stack.");
  InternetStackHelper stack;
  stack.Install(allNodesContainer);

  //-------------------------------------------------------------
  //              Install Devices & Assign IPaddress            -
  //-------------------------------------------------------------
  NS_LOG_INFO("-> Install Devices & Assign IPaddr.");
  // ======= build access network =======
  // LAN1
  buildCsmaNetwork(allNodesContainer, endLANNodeIDs[END_LAN1], endLANIPBases[END_LAN1], "5Mbps", 6560);
  // LAN2
  buildCsmaNetwork(allNodesContainer, endLANNodeIDs[END_LAN2], endLANIPBases[END_LAN2], "5Mbps", 6560);
  // LAN3
  buildCsmaNetwork(allNodesContainer, endLANNodeIDs[END_LAN3], endLANIPBases[END_LAN3], "5Mbps", 6560);
  // LAN4
  buildCsmaNetwork(allNodesContainer, endLANNodeIDs[END_LAN4], endLANIPBases[END_LAN4], "5Mbps", 6560);

  // ======= Connect edge routers with neighbors =======
  // edge router1
  p2pConnectToNeighbors(allNodesContainer, EDGE_ROUTER1, edgeRouter1Neighbors, edgeRouter1NeighborIPBase, "10Mbps", "2ms");
  // edge router2
  p2pConnectToNeighbors(allNodesContainer, EDGE_ROUTER2, edgeRouter2Neighbors, edgeRouter2NeighborIPBase, "10Mbps", "2ms");
  
  // ======= Connect core routers with neighbors =======
  // core router1
  p2pConnectToNeighbors(allNodesContainer, CORE_ROUTER1, coreRouter1Neighbors, coreRouter1NeighborIPBase, "20Mbps", "1ms");
  // core router2
  p2pConnectToNeighbors(allNodesContainer, CORE_ROUTER2, coreRouter2Neighbors, coreRouter2NeighborIPBase, "20Mbps", "1ms");

  //-------------------------------------------------------------
  //                          Routing                           -
  //-------------------------------------------------------------
  NS_LOG_INFO("-> Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //-------------------------------------------------------------
  //              Test Connectivity using V4Ping                -
  //-------------------------------------------------------------
  NS_LOG_INFO("-> Testing Connectivity.");
  testNodesConnectivity(allNodesContainer.Get(END_HOST1), allNodesContainer.Get(END_HOST8));
  testNodesConnectivity(allNodesContainer.Get(END_HOST8), allNodesContainer.Get(END_HOST1));
  //p2pHelper.EnablePcap ("second", accessCloud.Get(0)->GetId(), 0);
  //p2pHelper.EnablePcapAll("cybertwin");
  //PointToPointHelper p2pHelper;
  //CsmaHelper csmaHelper;
  //AsciiTraceHelper ascii;
  //p2pHelper.EnableAsciiAll(ascii.CreateFileStream("cybertwin1.tr"));
  //csmaHelper.EnableAsciiAll(ascii.CreateFileStream("cybertwin2.tr"));
  
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

/**
 * @brief 
 * 
 * @param allnodes 
 * @param nodeIds 
 * @param addrBase 
 * @param allNodesIPv4 
 * @param datarate 
 * @param delay 
 */
void buildCsmaNetwork(NodeContainer &allnodes,
                      const std::vector<uint32_t> &nodeIds,
                      IPaddrBase &addrBase,
                      const char* datarate,
                      const uint32_t delay)
{
  NodeContainer nodesCon;
  CsmaHelper csma;
  NetDeviceContainer csmaDevices;
  Ipv4AddressHelper address;
  
  for (uint32_t i=0; i<nodeIds.size(); i++)
  {
    nodesCon.Add(allnodes.Get(nodeIds[i]));
  }

  // set channel attribute
  csma.SetChannelAttribute ("DataRate", StringValue (datarate));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (delay)));

  csmaDevices = csma.Install(nodesCon);

  // assign ip
  address.SetBase(addrBase.first.c_str(), addrBase.second.c_str(), "0.0.0.100");
  address.Assign(csmaDevices);
}

/**
 * @brief 
 * 
 * @param allNodes 
 * @param center 
 * @param neighborIDs 
 * @param neighborsIPBase 
 * @param allNodesIPv4 
 * @param datarate 
 * @param delay 
 */
void p2pConnectToNeighbors(NodeContainer &allNodes,
                          const uint32_t centerID,
                          const std::vector<uint32_t> &neighborIDs,
                          std::vector<IPaddrBase> &neighborsIPBase,
                          const char* datarate,
                          const char* delay)
{
  uint32_t nNeighbors = neighborIDs.size();
  if (nNeighbors <= 0)
  {
    return;
  }

  PointToPointHelper p2pHelper;
  Ipv4AddressHelper address;
  Ptr<Node> center = allNodes.Get(centerID);
  Ptr<Node> neighbor = nullptr;
  IPaddrBase addrBase;
  NetDeviceContainer devices;

  p2pHelper.SetDeviceAttribute("DataRate", StringValue(datarate));
  p2pHelper.SetChannelAttribute("Delay", StringValue(delay));

  for (uint32_t i=0; i<nNeighbors; i++)
  {
    uint32_t neighborID = neighborIDs[i];
    neighbor = allNodes.Get(neighborID);
    addrBase = neighborsIPBase[i];
    
    // install p2p channel & devices
    devices = p2pHelper.Install(center, neighbor);

    // assign ip
    address.SetBase(addrBase.first.c_str(), addrBase.second.c_str());
    address.Assign(devices);
  }
}

void testNodesConnectivity(Ptr<Node> srcNode, Ptr<Node> dstNode)
{
  std::vector<Ipv4Address> srcIPList = getNodeIpv4List(dstNode); 
  std::vector<Ipv4Address> dstIPList = getNodeIpv4List(dstNode);
  uint32_t i=0;
  for (Ipv4Address dstIp:dstIPList)
  {
    V4PingHelper pingHelper(dstIp);
    std::cout<<"-> -> Ping Test Nodes Connectivity:\n\t from "<<srcIPList[0]<<" to "<<dstIp<<std::endl;
    pingHelper.SetAttribute("Interval", TimeValue(Seconds(1)));
    pingHelper.SetAttribute("Size", UintegerValue(100));
    ApplicationContainer pingContainer = pingHelper.Install(srcNode);
    pingContainer.Start(Seconds(10*(i+1)));
    pingContainer.Stop(Seconds(10*(i+1)+1));
    i++;
  }
}

std::vector<Ipv4Address> getNodeIpv4List(Ptr<Node> node)
{
  std::vector<Ipv4Address> ipList;
  Ptr<Ipv4L3Protocol> ipv4 = node->GetObject<Ipv4L3Protocol>();
  uint32_t nIf = ipv4->GetNInterfaces();
  for (uint32_t i=1; i<nIf; i++) // iteration start with 1 to skip 127.0.0.1
  {
    Ptr<Ipv4Interface> ipv4If = ipv4->GetInterface(i);
    uint32_t nAddr = ipv4If->GetNAddresses();
    for (uint32_t j=0; j<nAddr; j++)
    {
      Ipv4InterfaceAddress ifAddr = ipv4If->GetAddress(j);
      ipList.push_back(ifAddr.GetAddress());
    }
  }
  return ipList;
}