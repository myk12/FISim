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

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1   n2   n3   n4
//    point-to-point  |    |    |    |
//                    ================
//                      LAN 10.1.2.0

#define RECEIVER_ID 7777777
#define END_SWITCH_INTERVAL_MS 100
#define PACKET_SIZE 512
#define SEND_RATE 1000000 // 1Mbps
#define SYSTEM_MAX_RUNTIME 10 // 10s

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MobilityTest");

//******************************************************************************
//*                                 SenderApp
//******************************************************************************

class SenderApp : public Application
{
  public:
    static TypeId GetTypeId();
    SenderApp();
    ~SenderApp() override;

    void SendPacket()
    {
        // send packet at defined rate,
        // rate defined by SEND_RATE
        // using random time interval
        NS_LOG_FUNCTION(this);
        NS_LOG_DEBUG("[SenderApp::SendPacket] send packet to " << target_addr << ":"
                                                               << target_port);
        Ptr<Packet> packet = Create<Packet>(PACKET_SIZE);
        m_socket->Send(packet);
        // schedule next packet
        double interval = m_exponentialRandomVariable->GetValue();
        m_sendEvent = Simulator::Schedule(Seconds(interval), &SenderApp::SendPacket, this);
    }

    void SetTargetName(uint64_t name)
    {
        target_name = name;
    }

    void SetTargetAddress(Ipv4Address addr)
    {
        target_addr = addr;
    }

    void SetTargetPort(uint16_t port)
    {
        target_port = port;
    }

  private:
    void StartApplication() override;
    void StopApplication() override;

    Ptr<Socket> m_socket;
    uint64_t target_name;
    Ipv4Address target_addr;
    uint16_t target_port;

    // event id
    EventId m_sendEvent;

    // random variable
    Ptr<ExponentialRandomVariable> m_exponentialRandomVariable;
};

TypeId
SenderApp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SenderApp").SetParent<Application>().SetGroupName("Applications");
    return tid;
}

SenderApp::SenderApp()
{
    NS_LOG_FUNCTION(this);
    // create a exponential random variable
    m_exponentialRandomVariable = CreateObject<ExponentialRandomVariable>();

    // packet size / rate = interval between packet
    double interval = PACKET_SIZE * 8.0 / SEND_RATE;
    m_exponentialRandomVariable->SetAttribute("Mean", DoubleValue(interval));
}

SenderApp::~SenderApp()
{
    NS_LOG_FUNCTION(this);
}

void
SenderApp::StartApplication()
{
    NS_LOG_INFO("SenderApp::StartApplication");
    NS_LOG_FUNCTION(this);
    // send UDP packet
    if (!m_socket)
    {
        NS_LOG_DEBUG("[SenderApp::SenderApp] create socket");
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
    }

    // bind to any port
    NS_LOG_DEBUG("[SenderApp::SenderApp] bind to any port");
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 0);
    if (m_socket->Bind(local) == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    // set target address and port
    InetSocketAddress remote = InetSocketAddress(target_addr, target_port);
    if (m_socket->Connect(remote) == -1)
    {
        NS_FATAL_ERROR("Failed to connect socket");
    }

    // send packet every 1 second
    SendPacket();
}

void
SenderApp::StopApplication()
{
    NS_LOG_INFO("SenderApp::StopApplication");
    if (m_sendEvent.IsRunning())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }

    m_socket = 0;

    NS_LOG_DEBUG("SenderApp::StopApplication DONE");
}

//******************************************************************************
//*                                 ReceiverApp
//******************************************************************************

class ReceiverApp : public Application
{
  public:
    static TypeId GetTypeId();
    ReceiverApp();
    ~ReceiverApp() override;

    void HandleRead(Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);
        Ptr<Packet> packet;
        Address from;
        while ((packet = socket->RecvFrom(from)))
        {
            if (packet->GetSize() > 0)
            {
                NS_LOG_DEBUG("[Receiver_"
                             << m_recvId << "::HandleRead] Received " << packet->GetSize()
                             << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                             << " port " << InetSocketAddress::ConvertFrom(from).GetPort() << " at "
                             << Simulator::Now().GetSeconds() << "s");
                m_nPackets++;
            }

            if (m_nPackets == 3)
            {
                // add new route
                NS_LOG_DEBUG("[Receiver_" << m_recvId << "::HandleRead] add new route");
                Ipv4RoutingProtocol::m_name2addr[1234567] = Ipv4Address("10.1.2.3");
            }
        }
    }

    void SetPort(uint16_t port)
    {
        m_port = port;
    }

    void SetRecvId(uint32_t id)
    {
        m_recvId = id;
    }

    void SetTestApp(Ptr<Application> app)
    {
        m_testApp = app;
    }

    Ptr<Application> GetTestApp()
    {
        return m_testApp;
    }

  private:
    void StartApplication() override;
    void StopApplication() override;

    Ptr<Socket> m_socket;
    uint16_t m_port;
    uint32_t m_recvId;

    uint32_t m_nPackets;

    Ptr<Application> m_testApp;
};

TypeId
ReceiverApp::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ReceiverApp").SetParent<Application>().SetGroupName("Applications");
    return tid;
}

ReceiverApp::ReceiverApp()
{
    NS_LOG_FUNCTION(this);
}

ReceiverApp::~ReceiverApp()
{
    NS_LOG_FUNCTION(this);
}

void
ReceiverApp::StartApplication()
{
    NS_LOG_DEBUG("ReceiverApp::StartApplication");
    // lestin to UDP packet
    if (!m_socket)
    {
        NS_LOG_DEBUG("[Receiver_" << m_recvId << "::StartApplication] create socket");
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
    }

    // bind to port
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
    if (m_socket->Bind(local) == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    // set recv callback
    m_nPackets = 0;
    m_socket->SetRecvCallback(MakeCallback(&ReceiverApp::HandleRead, this));

    // announce name and address
    NS_LOG_DEBUG("[Receiver_" << m_recvId << "::StartApplication] announce name and address");
}

void
ReceiverApp::StopApplication()
{
    NS_LOG_DEBUG("ReceiverApp::StopApplication");

    if (m_socket)
    {
        m_socket->Close();
    }

    m_socket = 0;
}

void
EndTransmission(Ptr<Node> fromNode, Ptr<Node> toNode)
{
    if (!fromNode || !toNode)
    {
        NS_LOG_ERROR("EndTransmission: invalid node");
        return;
    }

    if (fromNode != nullptr)
    {
        NS_LOG_DEBUG("[EndTransmission] fromNode: " << fromNode->GetId());
        // find and stop old app
        Ptr<Application> app = nullptr;
        for (uint32_t i = 0; i < fromNode->GetNApplications(); i++)
        {
            app = fromNode->GetApplication(i);
            if (app->GetInstanceTypeId() == TypeId::LookupByName("ns3::ReceiverApp"))
            {
                NS_LOG_DEBUG("[EndTransmission] found old app");
                break;
            }
        }
        if (app)
        {
            NS_LOG_DEBUG("[EndTransmission] stop old app");
            // stop this app now
            app->SetStopTime(Seconds(0.0));
        }
    }

    if (toNode != nullptr)
    {
        NS_LOG_DEBUG("[EndTransmission] end transmission toNode: " << toNode->GetId());
        // create new app
        Ptr<ReceiverApp> receiverApp = CreateObject<ReceiverApp>();
        receiverApp->SetPort(10000);
        receiverApp->SetRecvId(1);

        // start this app after END_SWITCH_INTERVAL_MS ms
        NS_LOG_DEBUG("[EndTransmission] new app will start after " << END_SWITCH_INTERVAL_MS
                                                                   << "ms");
        receiverApp->SetStartTime(MilliSeconds(END_SWITCH_INTERVAL_MS));
        receiverApp->SetStopTime(Seconds(SYSTEM_MAX_RUNTIME));
    
        // install new app
        toNode->AddApplication(receiverApp);
    }
}

int
main(int argc, char* argv[])
{
    NS_LOG_UNCOND("======= ======= MobilityTest ======= =======");
    uint32_t nCsma = 2;
    // LogComponentEnable("Ipv4Header", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("TcpL4Protocol", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("UdpL4Protocol", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("Ipv4GlobalRouting", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("UdpSocketImpl", LOG_LEVEL_FUNCTION);
    // LogComponentEnable("UdpL4Protocol", LOG_LEVEL_FUNCTION);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_FUNCTION);
    LogComponentEnable("Ipv4GlobalRouting", LOG_LEVEL_DEBUG);
    LogComponentEnable("MobilityTest", LOG_LEVEL_DEBUG);

    NS_LOG_UNCOND("******* Creating nodes *******");
    NodeContainer p2pNodes;
    p2pNodes.Create(2);

    NodeContainer csmaNodes;
    csmaNodes.Add(p2pNodes.Get(1));
    csmaNodes.Create(nCsma);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer p2pDevices;
    p2pDevices = pointToPoint.Install(p2pNodes);

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));

    NetDeviceContainer csmaDevices;
    csmaDevices = csma.Install(csmaNodes);

    InternetStackHelper stack;
    stack.Install(p2pNodes.Get(0));
    stack.Install(csmaNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pInterfaces;
    p2pInterfaces = address.Assign(p2pDevices);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer csmaInterfaces;
    csmaInterfaces = address.Assign(csmaDevices);

    // install applications: sender and receiver

    Ptr<Node> sender = p2pNodes.Get(0);
    Ptr<Node> receiver1 = csmaNodes.Get(1);
    Ptr<Node> receiver2 = csmaNodes.Get(2);

    // create and install apps
    NS_LOG_UNCOND("******* Creating apps *******");
    Ptr<SenderApp> senderApp = CreateObject<SenderApp>();
    Ptr<ReceiverApp> receiverApp1 = CreateObject<ReceiverApp>();
    Ptr<ReceiverApp> receiverApp2 = CreateObject<ReceiverApp>();

    // config apps
    senderApp->SetTargetName(7777777);
    senderApp->SetTargetAddress(csmaInterfaces.GetAddress(1));
    senderApp->SetTargetPort(10000);

    receiverApp1->SetPort(10000);
    receiverApp1->SetRecvId(1);
    receiverApp2->SetPort(10000);
    receiverApp2->SetRecvId(2);

    // install apps
    NS_LOG_UNCOND("******* Installing apps *******");
    sender->AddApplication(senderApp);
    receiver1->AddApplication(receiverApp1);
    receiver2->AddApplication(receiverApp2);

    // set start and stop time
    NS_LOG_UNCOND("******* Setting start and stop time *******");
    senderApp->SetStartTime(Seconds(1.0));
    senderApp->SetStopTime(Seconds(10.0));
    receiverApp1->SetStartTime(Seconds(1.0));
    receiverApp1->SetStopTime(Seconds(10.0));
    receiverApp2->SetStartTime(Seconds(1.0));
    receiverApp2->SetStopTime(Seconds(10.0));

    NS_LOG_DEBUG("PopulateRoutingTables");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    NS_LOG_DEBUG("DONE PopulateRoutingTables");

    // pointToPoint.EnablePcapAll("second");

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
