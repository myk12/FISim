#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-routing-helper.h"

using namespace ns3;

// set log 
NS_LOG_COMPONENT_DEFINE("CCNSim");

int
main(int argc, char* argv[])
{
    // set log level
    LogComponentEnable("CCNSim", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("GlobalRouter", LOG_LEVEL_LOGIC);

    // construct topology
    NS_LOG_INFO("======= Simulation Content Centric Network =======");
    // create five nodes
    NS_LOG_INFO("---> Create 5 nodes");
    NodeContainer nodes;
    nodes.Create(5);

    // assign name to each node
    Names::Add("APNIC", nodes.Get(0));
    Names::Add("ARIN", nodes.Get(1));
    Names::Add("RIPE", nodes.Get(2));
    Names::Add("LACNIC", nodes.Get(3));
    Names::Add("AfriNIC", nodes.Get(4));

    // L3 protocol stack InternetStack
    // install protocol stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // L2 protocol stack NetDevice
    // create point-to-point links between nodes
    NS_LOG_INFO("---> Create point-to-point links between nodes");
    PointToPointHelper p2p;
    // link properties: 1Gbps, 10ms delay, 5% error rate
    p2p.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    p2p.SetChannelAttribute("Delay", StringValue("10ms"));

    // create net devices and channels for each pair of nodes
    NetDeviceContainer devices1 = p2p.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer devices2 = p2p.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer devices3 = p2p.Install(nodes.Get(2), nodes.Get(3));
    NetDeviceContainer devices4 = p2p.Install(nodes.Get(3), nodes.Get(4));
    NetDeviceContainer devices5 = p2p.Install(nodes.Get(4), nodes.Get(0));

    // assign IP addresses to each net device
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces1 = ipv4.Assign(devices1);
    ipv4.SetBase("20.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces2 = ipv4.Assign(devices2);
    ipv4.SetBase("30.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces3 = ipv4.Assign(devices3);
    ipv4.SetBase("40.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces4 = ipv4.Assign(devices4);
    ipv4.SetBase("50.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces5 = ipv4.Assign(devices5);

    // Routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // create applications UDP echo server
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(10.0));

    // create applications UDP echo client
    UdpEchoClientHelper echoClient(interfaces1.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps;
    for (int i = 1; i < 5; i++)
    {
        clientApps.Add(echoClient.Install(nodes.Get(i)));
    }
    clientApps.Start(Seconds(2.0));

    // iterate over all nodes and print their names
    NodeList::Iterator listEnd = NodeList::End();
    for (NodeList::Iterator iter = NodeList::Begin(); iter != listEnd; ++iter)
    {
        Ptr<Node> node = *iter;
        NS_LOG_INFO("Node: " << Names::FindName(node));
    }

    // print routing tables
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("routing-tables", std::ios::out);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(2.0), routingStream);

    // run simulation
    NS_LOG_INFO("---> Run simulation");
    Simulator::Run();

    // cleanup
    Simulator::Destroy();
}
