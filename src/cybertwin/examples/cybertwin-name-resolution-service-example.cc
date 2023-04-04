#include "ns3/core-module.h"
#include "ns3/cybertwin-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/cybertwin-name-resolution-service.h"
#include <iostream>

using namespace ns3;

int
main(int argc, char* argv[])
{

    LogComponentEnable("NameResolutionService", LOG_LEVEL_INFO);
    //LogComponentEnable("Socket", LOG_LEVEL_DEBUG);
    
    NodeContainer hosts;
    hosts.Create(2);

    PointToPointHelper p2pHelper;
    NetDeviceContainer devices;
    InternetStackHelper stack;
    Ipv4AddressHelper address;
    Ipv4InterfaceContainer ifs;
    
    stack.Install(hosts);
    devices = p2pHelper.Install(hosts);
    address.SetBase("10.1.1.0", "255.255.255.0");
    ifs = address.Assign(devices);

    NameResolutionService upNodeApp;
    Ptr<Node> serverNode = hosts.Get(0);
    serverNode->AddApplication(&upNodeApp);

    NameResolutionService downNodeApp(ifs.GetAddress(0));
    Ptr<Node> clientNode = hosts.Get(1);
    clientNode->AddApplication(&downNodeApp);

    upNodeApp.SetStartTime(Seconds(1));
    //upNodeApp.SetStopTime(Seconds(10));
    downNodeApp.SetStartTime(Seconds(2));
    //downNodeApp.SetStopTime(Seconds(9));

    uint16_t port = 8900;
    CYBERTWIN_INTERFACE_t interface;

    AsciiTraceHelper ascii;
    p2pHelper.EnableAsciiAll(ascii.CreateFileStream("cybertwin-name-resolution-example.tr"));
    /* ... */

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
