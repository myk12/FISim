#include "cybertwin-client.h"

#include "ns3/address.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinClient");

NS_OBJECT_ENSURE_REGISTERED(CybertwinClient);
NS_OBJECT_ENSURE_REGISTERED(CybertwinConnClient);
NS_OBJECT_ENSURE_REGISTERED(CybertwinBulkClient);

TypeId
CybertwinClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CybertwinClient")
            .SetParent<Application>()
            .SetGroupName("cybertwin")
            .AddConstructor<CybertwinClient>()
            .AddAttribute("Socket",
                          "The pointer of the socket connected to the corresponding cybertwin",
                          PointerValue(),
                          MakePointerAccessor(&CybertwinClient::m_socket),
                          MakePointerChecker<Socket>())
            .AddAttribute("LocalCuid",
                          "The CUID of this  device",
                          UintegerValue(),
                          MakeUintegerAccessor(&CybertwinClient::m_localCuid),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("PeerCuid",
                          "The CUID of the peer device",
                          UintegerValue(),
                          MakeUintegerAccessor(&CybertwinClient::m_peerCuid),
                          MakeUintegerChecker<uint64_t>());
    return tid;
}

CybertwinClient::CybertwinClient()
    : m_socket(nullptr)
{
}

void
CybertwinClient::DoDispose()
{
    m_socket = nullptr;
    Application::DoDispose();
}

void
CybertwinClient::RecvPacket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            break;
        }
        CybertwinPacketHeader header;
        packet->PeekHeader(header);
        NS_LOG_DEBUG("Client received packet with header " << header << " from " << from);
    }
}

TypeId
CybertwinConnClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CybertwinConnClient")
            .SetParent<CybertwinClient>()
            .SetGroupName("cybertwin")
            .AddConstructor<CybertwinConnClient>()
            .AddAttribute("LocalAddress",
                          "The address on which to bind the socket",
                          AddressValue(),
                          MakeAddressAccessor(&CybertwinConnClient::m_localAddr),
                          MakeAddressChecker())
            .AddAttribute("LocalPort",
                          "The port on which the application sends data",
                          UintegerValue(80),
                          MakeUintegerAccessor(&CybertwinConnClient::m_localPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("EdgeAddress",
                          "The address of the edge server serving this  network",
                          AddressValue(),
                          MakeAddressAccessor(&CybertwinConnClient::m_edgeAddr),
                          MakeAddressChecker())
            .AddAttribute("EdgePort",
                          "The port on which the edge server listens for incoming data",
                          UintegerValue(443),
                          MakeUintegerAccessor(&CybertwinConnClient::m_edgePort),
                          MakeUintegerChecker<uint16_t>());
    return tid;
}

CybertwinConnClient::CybertwinConnClient()
    : m_controllerSocket(nullptr)
{
    NS_LOG_FUNCTION(this);
}

CybertwinConnClient::~CybertwinConnClient()
{
    NS_LOG_FUNCTION(this);
}

void
CybertwinConnClient::DoDispose()
{
    NS_LOG_FUNCTION(this->GetTypeId());
    if (!Simulator::IsFinished())
    {
        StopApplication();
    }
    m_controllerSocket = nullptr;
    CybertwinClient::DoDispose();
}

void
CybertwinConnClient::StartApplication()
{
    NS_LOG_FUNCTION(this->GetTypeId());
    if (!m_controllerSocket)
    {
        m_controllerSocket =
            Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        DoSocketMethod(&Socket::Bind, m_controllerSocket, m_localAddr, m_localPort);

        m_controllerSocket->SetConnectCallback(
            MakeCallback(&CybertwinConnClient::ControllerConnectSucceededCallback, this),
            MakeCallback(&CybertwinConnClient::ControllerConnectFailedCallback, this));
        // TCP socket base will try to connect every 3 seconds. Inherit TcpSocketBase to modify this
        DoSocketMethod(&Socket::Connect, m_controllerSocket, m_edgeAddr, m_edgePort);
        NS_LOG_DEBUG("--[Host-Conn]: Connecting to CybertwinController");
    }
}

void
CybertwinConnClient::StopApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_socket)
    {
        DisconnectCybertwin();
        m_socket->Close();
    }

    if (m_controllerSocket)
    {
        m_controllerSocket->Close();
    }
}

void
CybertwinConnClient::ControllerConnectSucceededCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_controllerSocket->SetRecvCallback(
        MakeCallback(&CybertwinConnClient::RecvFromControllerCallback, this));
    Simulator::ScheduleNow(&CybertwinConnClient::GenerateCybertwin, this);
}

void
CybertwinConnClient::ControllerConnectFailedCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_ERROR(
        "--[Host-Conn]: Failed to connect to CybertwinController, errno: " << socket->GetErrno());
}

void
CybertwinConnClient::RecvFromControllerCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            break;
        }

        CybertwinControllerHeader ctrlHeader;
        packet->RemoveHeader(ctrlHeader);

        switch (ctrlHeader.GetMethod())
        {
        case CYBERTWIN_CONTROLLER_SUCCESS:
            NS_LOG_DEBUG("--[Host-Conn]: Cybertwin generated successfully");
            m_localCuid = ctrlHeader.GetCybertwinID();
            m_cybertwinPort = ctrlHeader.GetCybertwinPort();
            ConnectCybertwin();
            break;
        case CYBERTWIN_CONTROLLER_ERROR:
            NS_LOG_INFO("--[Host-Conn]: Failed to generate a cybertwin, retrying");
            Simulator::Schedule(Seconds(2.), &CybertwinConnClient::GenerateCybertwin, this);
            break;
        default:
            NS_LOG_INFO("--[Host-Conn]: Invalid response from controller, retrying");
            Simulator::Schedule(Seconds(2.), &CybertwinConnClient::GenerateCybertwin, this);
            break;
        }
    }
}

void
CybertwinConnClient::GenerateCybertwin()
{
    NS_LOG_FUNCTION(this);
    CybertwinControllerHeader ctrlHeader;

    // temporary, generate a device id
    srandom((unsigned int)Simulator::Now().GetNanoSeconds());

    ctrlHeader.SetMethod(CYBERTWIN_CREATE);
    ctrlHeader.SetDeviceName(rand() % 1333333);
    ctrlHeader.SetNetworkType(rand() % 3);

    Ptr<Packet> connPacket = Create<Packet>(0);
    connPacket->AddHeader(ctrlHeader);

    // TODO: check if the packet is sent successfully?
    m_controllerSocket->Send(connPacket);
    NS_LOG_DEBUG("--[Host-Conn]: Request to generate a cybertwin sent");
}

void
CybertwinConnClient::ConnectCybertwin()
{
    NS_LOG_FUNCTION(this);
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        DoSocketMethod(&Socket::Bind, m_socket, m_localAddr, m_localPort + 1);
    }
    m_socket->SetConnectCallback(
        MakeCallback(&CybertwinConnClient::CybertwinConnectSucceededCallback, this),
        MakeCallback(&CybertwinConnClient::CybertwinConnectFailedCallback, this));
    m_socket->SetCloseCallbacks(
        MakeCallback(&CybertwinConnClient::CybertwinCloseSucceededCallback, this),
        MakeCallback(&CybertwinConnClient::CybertwinCloseFailedCallback, this));
    DoSocketMethod(&Socket::Connect, m_socket, m_edgeAddr, m_cybertwinPort);
    NS_LOG_DEBUG("--[Host-Conn]: Request to connect to the cybertwin sent");
}

void
CybertwinConnClient::DisconnectCybertwin()
{
    NS_LOG_FUNCTION(this);
    CybertwinControllerHeader ctrlHeader;
    ctrlHeader.SetMethod(CYBERTWIN_REMOVE);
    ctrlHeader.SetCybertwinID(m_localCuid);

    Ptr<Packet> connPacket = Create<Packet>(0);
    connPacket->AddHeader(ctrlHeader);

    m_controllerSocket->Send(connPacket);
    NS_LOG_DEBUG("--[Host-Conn]: Request to disconnect to the cybertwin sent");
}

void
CybertwinConnClient::CybertwinConnectSucceededCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    socket->SetRecvCallback(MakeCallback(&CybertwinConnClient::RecvPacket, this));

    // Set attributes of other applications
    uint32_t appNums = GetNode()->GetNApplications();
    for (uint32_t i = 0; i < appNums; ++i)
    {
        Ptr<Application> curApp = GetNode()->GetApplication(i);
        if (curApp != this)
        {
            NS_LOG_DEBUG("--[Host-Conn]: Setting attributes for " << curApp << " at "
                                                                  << Simulator::Now());
            curApp->SetAttribute("Socket", PointerValue(m_socket));
            curApp->SetAttribute("LocalCuid", UintegerValue(m_localCuid));
            curApp->SetAttribute("PeerCuid", UintegerValue(m_peerCuid));
        }
    }
}

void
CybertwinConnClient::CybertwinConnectFailedCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Simulator::Schedule(Seconds(1.), &CybertwinConnClient::ConnectCybertwin, this);
}

void
CybertwinConnClient::CybertwinCloseSucceededCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                              MakeNullCallback<void, Ptr<Socket>>());
    m_socket = nullptr;
}

void
CybertwinConnClient::CybertwinCloseFailedCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket->GetErrno());
}

TypeId
CybertwinBulkClient::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinBulkClient")
                            .SetParent<CybertwinClient>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinBulkClient>()
                            .AddAttribute("MaxBytes",
                                          "The total number of bytes to send. 0 means no limit",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&CybertwinBulkClient::m_maxBytes),
                                          MakeUintegerChecker<uint64_t>())
                            .AddAttribute("PacketSize",
                                          "The size of a single packet",
                                          UintegerValue(2048),
                                          MakeUintegerAccessor(&CybertwinBulkClient::m_packetSize),
                                          MakeUintegerChecker<uint32_t>(21));
    return tid;
}

CybertwinBulkClient::CybertwinBulkClient()
{
    NS_LOG_FUNCTION(this);
}

CybertwinBulkClient::~CybertwinBulkClient()
{
    NS_LOG_FUNCTION(this);
}

void
CybertwinBulkClient::StartApplication()
{
    NS_LOG_FUNCTION(this->GetTypeId());
    NS_LOG_DEBUG("--[Host-Bulk]: StartApplication called at " << Simulator::Now());
    SendData();
}

void
CybertwinBulkClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

// void
// CybertwinBulkClient::DoInitialize()
// {
//     NS_LOG_FUNCTION(this);
//     // Skip Application::DoInitialize
//     Object::DoInitialize();
// }

void
CybertwinBulkClient::DoDispose()
{
    NS_LOG_FUNCTION(this->GetTypeId());
    if (m_socket)
    {
        m_socket = nullptr;
    }
    CybertwinClient::DoDispose();
}

void
CybertwinBulkClient::SendData()
{
    if (!m_socket)
    {
        // block until CybertwinConnClient completes connection
        Simulator::Schedule(Seconds(0.1), &CybertwinBulkClient::SendData, this);
        return;
    }

    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("--[Host-Bulk]: Started sending data at " << Simulator::Now());
    while (m_maxBytes == 0 || m_sentBytes < m_maxBytes)
    {
        // uint64_t to allow the comparison later
        uint64_t toSend = m_packetSize;
        if (m_maxBytes > 0)
        {
            toSend = std::min(toSend, m_maxBytes - m_sentBytes);
        }

        Ptr<Packet> packet;
        if (m_unsentPacket)
        {
            NS_LOG_DEBUG("Resending an unsent packet");
            packet = m_unsentPacket;
            toSend = packet->GetSize();
        }
        else
        {
            CybertwinPacketHeader header;
            header.SetSize(toSend);
            header.SetCmd(CYBERTWIN_SEND);
            header.SetSrc(m_localCuid);
            header.SetDst(m_peerCuid);
            // NS_LOG_DEBUG("Client sent packet with header: " << header);
            NS_ABORT_IF(toSend < header.GetSerializedSize());
            packet = Create<Packet>(toSend - header.GetSerializedSize());
            packet->AddHeader(header);
        }

        int actual = m_socket->Send(packet);
        if ((unsigned)actual == toSend)
        {
            m_sentBytes += actual;
            m_unsentPacket = nullptr;
        }
        else if (actual == -1)
        {
            NS_LOG_DEBUG("Unable to send packet; caching for later attempt");
            m_unsentPacket = packet;
            break;
        }
        else if (actual > 0 && (unsigned)actual < toSend)
        {
            // A Linux socket (non-blocking, such as in DCE) may return
            // a quantity less than the packet size.  Split the packet
            // into two, trace the sent packet, save the unsent packet
            NS_LOG_DEBUG("Packet size: " << packet->GetSize() << "; sent: " << actual
                                         << "; fragment saved: " << toSend - (unsigned)actual);
            Ptr<Packet> unsent = packet->CreateFragment(actual, (toSend - (unsigned)actual));
            m_sentBytes += actual;
            m_unsentPacket = unsent;
            break;
        }
        else
        {
            NS_FATAL_ERROR("Unexpected return value from m_socket->Send ()");
        }
    }
}

} // namespace ns3