#include "ns3/applications-module.h"
#include "ns3/ccn-consumer-app.h"
#include "ns3/ccn-producer-app.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <unordered_map>
#include <vector>

using namespace ns3;

// set log
NS_LOG_COMPONENT_DEFINE("CCNSim");

int
main(int argc, char* argv[])
{
    // set log level
    LogComponentEnable("CCNSim", LOG_LEVEL_INFO);
    LogComponentEnable("CCNProducerApp", LOG_LEVEL_FUNCTION);
    LogComponentEnable("CCNConsumerApp", LOG_LEVEL_FUNCTION);
    LogComponentEnable("CCNL4Protocol", LOG_LEVEL_FUNCTION);
    LogComponentEnable("CCNContentProducer", LOG_LEVEL_FUNCTION);
    LogComponentEnable("CCNContentConsumer", LOG_LEVEL_FUNCTION);

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
    NS_LOG_INFO("---> Install protocol stack");
    InternetStackHelper stack;
    stack.SetCCNStackInstall(true);
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
    NS_LOG_INFO("---> Assign IP addresses to each net device");
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
    NS_LOG_INFO("---> Populate routing tables");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Construct node name to IpAddress mapping
    NS_LOG_INFO("---> Construct node name to IpAddress mapping");
    std::vector<std::pair<std::string, Ipv4Address>> nodeNameToIp;
    nodeNameToIp.push_back(std::make_pair("APNIC", interfaces1.GetAddress(0)));
    nodeNameToIp.push_back(std::make_pair("ARIN", interfaces1.GetAddress(1)));
    nodeNameToIp.push_back(std::make_pair("RIPE", interfaces2.GetAddress(1)));
    nodeNameToIp.push_back(std::make_pair("LACNIC", interfaces3.GetAddress(1)));
    nodeNameToIp.push_back(std::make_pair("AfriNIC", interfaces4.GetAddress(1)));

    // for each node, Insert the node name to IpAddress mapping
    // to CCNL4Protocol
    for (int i = 0; i < 5; i++)
    {
        Ptr<Node> node = nodes.Get(i);
        Ptr<CCNL4Protocol> ccnl4 = node->GetObject<CCNL4Protocol>();
        NS_ASSERT_MSG(ccnl4, "CCNL4Protocol not found");
        ccnl4->AddContentPrefixToHostAddress(nodeNameToIp);
    }

    // create CCN content producer
    NS_LOG_INFO("---> Create CCN content producer");
    Ptr<CCNProducerApp> producer = CreateObject<CCNProducerApp>();
    producer->SetAttribute("ContentName", StringValue("content"));
    producer->SetAttribute("ContentFile", StringValue("/home/ubuntu/FISim/FIA/CCN/content.txt"));
    producer->Install(nodes.Get(0));
    producer->SetStartTime(Seconds(1.0));

    // create CCN content consumer
    NS_LOG_INFO("---> Create CCN content consumer");
    Ptr<CCNConsumerApp> consumer = CreateObject<CCNConsumerApp>();
    consumer->SetAttribute("ContentNames", StringValue("APNIC/content"));
    consumer->Install(nodes.Get(1));
    consumer->SetStartTime(Seconds(2.0));

    // iterate over all nodes and print their names
    NodeList::Iterator listEnd = NodeList::End();
    for (NodeList::Iterator iter = NodeList::Begin(); iter != listEnd; ++iter)
    {
        Ptr<Node> node = *iter;
        NS_LOG_INFO("Node: " << Names::FindName(node));
    }

    // print routing tables
    Ptr<OutputStreamWrapper> routingStream =
        Create<OutputStreamWrapper>("routing-tables", std::ios::out);
    Ipv4RoutingHelper::PrintRoutingTableAllAt(Seconds(2.0), routingStream);

    // run simulation
    NS_LOG_INFO("---> Run simulation");
    Simulator::Run();

    // cleanup
    Simulator::Destroy();
}
