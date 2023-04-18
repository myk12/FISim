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
#include "ns3/cybertwin-name-resolution-service.h"
#include "ns3/cybertwin-node-endhost.h"
#include "ns3/cybertwin-node-edgeserver.h"
#include "ns3/cybertwin-node-coreserver.h"
#include "cybertwin-topology.h"

#include <vector>
#include <iostream>
#include <sstream>
#include <unordered_map>

// Network Topology, Cybertwin: v1.0
//          
//
#define TOPOLOGY_GRAPH (\
"          - Cybertwin Network Simulation: version 1 -            \n"\
"\n"\
"                    [core1]                  [core2]             \n" \
"                       |                        |                \n" \
"              _________| _______________________|________        \n" \
"                |            |            |           |          \n" \
"                |            |            |           |          \n" \
"             [edge1]       [edge2]     [edge3]      [edge4]      \n" \
"                |            |            |           |          \n" \
"             ___|___      ___|___      ___|___     ___|___       \n" \
"             |     |      |     |      |     |     |     |       \n" \
"             |     |      |     |      |     |     |     |       \n" \
"           [h1]   [h2]  [h3]   [h4]  [h5]   [h6]  [h7]   [h8]    \n" \
)
//
//

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Cybertwinv1");

int 
main (int argc, char *argv[])
{
  LogComponentEnable("Cybertwinv1", LOG_LEVEL_INFO);
  LogComponentEnable ("V4Ping", LOG_LEVEL_DEBUG);
  LogComponentEnable("CybertwinEdgeServer", LOG_LEVEL_DEBUG);
  LogComponentEnable("CybertwinCoreServer", LOG_LEVEL_DEBUG);
  LogComponentEnable("CybertwinEndHost", LOG_LEVEL_DEBUG);
  LogComponentEnable("CybertwinClient", LOG_LEVEL_DEBUG);
  LogComponentEnable("CybertwinEdge", LOG_LEVEL_DEBUG);

  //***********************************************************************
  //*                   Building Topology                                 *
  //***********************************************************************
  NS_LOG_UNCOND("[1] ******* Building Topology *******\n\n");
  NS_LOG_UNCOND(TOPOLOGY_GRAPH);
  NodeContainer allNodesContainer;
  std::vector<std::vector<Ipv4Address>> allNodesIpv4Addresses;

  for (int32_t i=0; i<MAX_NODE_NUM; i++)
  {
    if (END_HOST1 <= i && i <= END_HOST8)
    {// create end host
      Ptr<CybertwinEndHost> endhost = CreateObject<CybertwinEndHost>();
      allNodesContainer.Add(endhost);
    }else if (((EDGE_SERVER1 <= i) && (i <= EDGE_SERVER2)) || ((EDGE_SERVER3 <= i) && (i <= EDGE_SERVER4)))
    {// create edge server
      Ptr<CybertwinEdgeServer> edgeServer = CreateObject<CybertwinEdgeServer>();
      allNodesContainer.Add(edgeServer);
    }else if (CORE_SERVER1 <= i && i<= CORE_SERVER2)
    {// create core server
      Ptr<CybertwinCoreServer> coreServer = CreateObject<CybertwinCoreServer>();
      allNodesContainer.Add(coreServer);
    }else
    {// create router
      allNodesContainer.Create(1);
    }
  }

  // Install Network Stack 
  NS_LOG_INFO("-> Install Network Stack.");
  InternetStackHelper stack;
  stack.Install(allNodesContainer);

  // Install Devices and Assign Ipaddress
  NS_LOG_INFO("-> Install Devices & Assign IPaddr.");
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

  allNodesIpv4Addresses = getNodesIpv4List(allNodesContainer);

  // Routing
  NS_LOG_INFO("-> Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //-------------------------------------------------------------
  //              Test Connectivity using V4Ping                -
  //-------------------------------------------------------------
  NS_LOG_INFO("-> Testing Connectivity.");
  testNodesConnectivity(allNodesContainer.Get(END_HOST1), allNodesContainer.Get(END_HOST8));
  testNodesConnectivity(allNodesContainer.Get(END_HOST8), allNodesContainer.Get(END_HOST1));

  // Start Up
  NS_LOG_INFO("-> Install Software.");
  for (int32_t i=0; i<MAX_NODE_NUM; i++)
  {
    Ptr<Node> node = allNodesContainer.Get(i);
    switch (i)
    {
    case END_HOST1:
    case END_HOST2:
      DynamicCast<CybertwinEndHost>(node)->Setup(allNodesIpv4Addresses[EDGE_SERVER1][0]);
      break;
    case END_HOST3:
    case END_HOST4:
      DynamicCast<CybertwinEndHost>(node)->Setup(allNodesIpv4Addresses[EDGE_SERVER2][0]);
      break;
    case END_HOST5:
    case END_HOST6:
      DynamicCast<CybertwinEndHost>(node)->Setup(allNodesIpv4Addresses[EDGE_SERVER3][0]);
      break;
    case END_HOST7:
    case END_HOST8:
      DynamicCast<CybertwinEndHost>(node)->Setup(allNodesIpv4Addresses[EDGE_SERVER4][0]);
      break;
    case EDGE_SERVER1:
    case EDGE_SERVER2:
      DynamicCast<CybertwinEdgeServer>(node)->Setup(allNodesIpv4Addresses[CORE_SERVER1][0]);
      break;
    case EDGE_SERVER3:
    case EDGE_SERVER4:
      DynamicCast<CybertwinEdgeServer>(node)->Setup(allNodesIpv4Addresses[CORE_SERVER2][0]);
      break;
    case CORE_SERVER1:
    case CORE_SERVER2:
      DynamicCast<CybertwinCoreServer>(node)->Setup();
      break;
    default:
      break;
    }
  }

  NS_LOG_DEBUG("Start Simulatoin.");

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}