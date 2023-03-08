#include "ns3/core-module.h"
#include "ns3/cybertwin-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TestEndToEdge");

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//

using namespace ns3;

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Packet::EnablePrinting();

    Time::SetResolution(Time::NS);
    LogComponentEnable("CybertwinEdge", LOG_LEVEL_ALL);
    LogComponentEnable("CybertwinClient", LOG_LEVEL_ALL);
    LogComponentEnable("Cybertwin", LOG_LEVEL_ALL);

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    CybertwinHelper connClient("ns3::CybertwinConnClient");
    connClient.SetAttribute("PeerCuid", UintegerValue(1234));
    connClient.SetAttribute("LocalAddress", AddressValue(interfaces.GetAddress(0)));
    connClient.SetAttribute("EdgeAddress", AddressValue(interfaces.GetAddress(1)));
    ApplicationContainer clientConnApp = connClient.Install(nodes.Get(0));

    CybertwinHelper bulkClient("ns3::CybertwinBulkClient");
    ApplicationContainer clientBulkApp = bulkClient.Install(nodes.Get(0));
    clientConnApp.Start(Seconds(1.0));
    clientBulkApp.Start(Seconds(1.1));

    CybertwinHelper edgeServer("ns3::CybertwinController");
    edgeServer.SetAttribute("LocalAddress", AddressValue(interfaces.GetAddress(1)));
    ApplicationContainer edgeCloud = edgeServer.Install(nodes.Get(1));
    edgeCloud.Start(Seconds(0.0));
    // edgeCloud.Stop(Seconds(15.0));

    // Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
