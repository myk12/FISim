#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

int main(int argc, char *argv[]) {
    double simTime = 10.0;

    // open log
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<EpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create LTE nodes (eNodeB and UE)
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(1);     // one eNodeB
    ueNodes.Create(2);      // two UE

    // configure eNodeB and UE devices and install them to the nodes
    MobilityHelper mobility;

    // eNodeB use constant position mobility model
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);

    // UE use random walk 2D mobility model
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.Install(ueNodes);

    // Install LTE devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

    // Install the IP stack on the UEs
    InternetStackHelper internet;
    internet.Install(ueNodes);

    // Assign IP address to UEs
    Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address(ueLteDevs);

    // Assign IP address to eNodeB
    for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
        Ptr<Node> ueNode = ueNodes.Get(i);
        Ptr<Ipv4StaticRouting> ueStaticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(
            ueNode->GetObject<Ipv4>()->GetRoutingProtocol());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // Create a point-to-point link between eNodeB and PGW
    NodeContainer csmaNodes;
    csmaNodes.Add(pgw);
    csmaNodes.Create(1); // one CSMA node

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("1Gbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer csmaDevs = csma.Install(csmaNodes);

    // Install the IP stack on the CSMA node
    internet.Install(csmaNodes.Get(1));

    // Assign IP address to the CSMA node
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaIfaces = ipv4.Assign(csmaDevs);

    // 安装 UDP Echo Server
    uint16_t echoPort = 9;
    UdpEchoServerHelper echoServer(echoPort);
    ApplicationContainer serverApps = echoServer.Install(csmaNodes.Get(1));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simTime));

    // 安装 UDP Echo Client
    UdpEchoClientHelper echoClient(csmaIfaces.GetAddress(1), echoPort);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100));
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = echoClient.Install(ueNodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTime));

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}   