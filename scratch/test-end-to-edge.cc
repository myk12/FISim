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

    Time::SetResolution(Time::NS);
    LogComponentEnable("CybertwinController", LOG_LEVEL_DEBUG);
    LogComponentEnable("CybertwinBulkClient", LOG_LEVEL_DEBUG);
    LogComponentEnable("Cybertwin", LOG_LEVEL_DEBUG);

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

    CybertwinBulkClientHelper bulkClient(1234,
                                         4321,
                                         interfaces.GetAddress(0),
                                         interfaces.GetAddress(1));
    ApplicationContainer clientApps = bulkClient.Install(nodes.Get(0));
    clientApps.Start(Seconds(1.0));
    // clientApps.Stop(Seconds(10.0));

    CybertwinEdgeServerHelper edgeServer(interfaces.GetAddress(1));
    ApplicationContainer edgeCloud = edgeServer.Install(nodes.Get(1));
    edgeCloud.Start(Seconds(0.0));
    // edgeCloud.Stop(Seconds(10.0));

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
