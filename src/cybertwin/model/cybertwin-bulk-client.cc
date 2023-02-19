#include "cybertwin-bulk-client.h"

#include "ns3/address.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinBulkClient");
NS_OBJECT_ENSURE_REGISTERED(CybertwinBulkClient);

TypeId
CybertwinBulkClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CybertwinBulkClient")
            .SetParent<Application>()
            .SetGroupName("cybertwin")
            .AddConstructor<CybertwinBulkClient>()
            .AddAttribute("SendSize",
                          "The amount of data to send each time",
                          UintegerValue(512),
                          MakeUintegerAccessor(&CybertwinBulkClient::m_sendSize),
                          MakeUintegerChecker<uint32_t>(1))
            .AddAttribute("EdgeAddress",
                          "The address of the edge server serving this LAN",
                          AddressValue(),
                          MakeAddressAccessor(&CybertwinBulkClient::m_edgeAddr),
                          MakeAddressChecker())
            .AddAttribute("EdgePort",
                          "The port on which the edge server listens for incoming data",
                          UintegerValue(443),
                          MakeUintegerAccessor(&CybertwinBulkClient::m_edgePort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("LocalAddress",
                          "The address on which to bind the socket",
                          AddressValue(),
                          MakeAddressAccessor(&CybertwinBulkClient::m_localAddr),
                          MakeAddressChecker())
            .AddAttribute("LocalPort",
                          "The port on which the application sends data",
                          UintegerValue(80),
                          MakeUintegerAccessor(&CybertwinBulkClient::m_localPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("LocalGuid",
                          "The Guid of this device",
                          UintegerValue(),
                          MakeUintegerAccessor(&CybertwinBulkClient::m_localGuid),
                          MakeUintegerChecker<uint64_t>())
            .AddAttribute("PeerGuid",
                          "The Guid of the peer device",
                          UintegerValue(),
                          MakeUintegerAccessor(&CybertwinBulkClient::m_peerGuid),
                          MakeUintegerChecker<uint64_t>());
    return tid;
}

CybertwinBulkClient::CybertwinBulkClient()
    : m_socket(nullptr)
{
    NS_LOG_FUNCTION(this);
}

CybertwinBulkClient::~CybertwinBulkClient()
{
    NS_LOG_FUNCTION(this);
}

void
CybertwinBulkClient::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_socket = nullptr;
    Application::DoDispose();
}

void
CybertwinBulkClient::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        if (Ipv4Address::IsMatchingType(m_localAddr))
        {
            const Ipv4Address ipv4 = Ipv4Address::ConvertFrom(m_localAddr);
            const InetSocketAddress inetSocket = InetSocketAddress(ipv4, m_localPort);
            m_socket->Bind(inetSocket);
            NS_LOG_DEBUG("client bind to " << ipv4 << ":" << m_localPort);
        }
        else if (Ipv6Address::IsMatchingType(m_localAddr))
        {
            const Ipv6Address ipv6 = Ipv6Address::ConvertFrom(m_localAddr);
            const Inet6SocketAddress inet6Socket = Inet6SocketAddress(ipv6, m_localPort);
            m_socket->Bind(inet6Socket);
            NS_LOG_DEBUG("client bind to " << ipv6 << ":" << m_localPort);
        }
        Connect();
    }
}

void
CybertwinBulkClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
CybertwinBulkClient::Connect()
{
    NS_LOG_FUNCTION(this);
    m_socket->SetConnectCallback(MakeCallback(&CybertwinBulkClient::ConnectionSucceeded, this),
                                 MakeCallback(&CybertwinBulkClient::ConnectionFailed, this));

    if (Ipv4Address::IsMatchingType(m_edgeAddr))
    {
        const Ipv4Address ipv4 = Ipv4Address::ConvertFrom(m_edgeAddr);
        const InetSocketAddress inetSocket = InetSocketAddress(ipv4, m_edgePort);
        m_socket->Connect(inetSocket);
        NS_LOG_DEBUG("client connecting to " << ipv4 << ":" << m_edgePort);
    }
    else if (Ipv6Address::IsMatchingType(m_edgeAddr))
    {
        const Ipv6Address ipv6 = Ipv6Address::ConvertFrom(m_edgeAddr);
        const Inet6SocketAddress inet6Socket = Inet6SocketAddress(ipv6, m_edgePort);
        m_socket->Connect(inet6Socket);
        NS_LOG_DEBUG("client connecting to " << ipv6 << ":" << m_edgePort);
    }
}

void
CybertwinBulkClient::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> connPacket = Create<Packet>(0);
    //CybertwinPacketHeader header(m_localGuid, m_localGuid, 0, 1);
    CybertwinControllerHeader header;
    header.SetMethod(CREATE);

    connPacket->AddHeader(header);
    m_socket->Send(connPacket);
    NS_LOG_DEBUG("completed TCP handshake, connection packet sent");
    //sendMethod();
}

void
CybertwinBulkClient::sendMethod()
{
    NS_LOG_DEBUG("Clinet Send packet.");
    uint16_t method = 0;
    CybertwinControllerHeader header;

    srand (time(NULL));
    header.SetMethod(rand() % 3);

    Ptr<Packet> connPacket = Create<Packet>(0);
    connPacket->AddHeader(header);
    m_socket->Send(connPacket);

    Simulator::Schedule(Seconds(3.), &CybertwinBulkClient::sendMethod, this);
}

void
CybertwinBulkClient::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket->GetErrno());
}

} // namespace ns3