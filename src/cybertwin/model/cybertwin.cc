#include "cybertwin.h"

#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("Cybertwin");
NS_OBJECT_ENSURE_REGISTERED(Cybertwin);

TypeId
Cybertwin::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Cybertwin")
                            .SetParent<Application>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<Cybertwin>();
    return tid;
}

Cybertwin::Cybertwin()
    : m_cybertwinId(0),
      m_localSocket(nullptr),
      m_globalSocket(nullptr)
{
}

Cybertwin::Cybertwin(CYBERTWINID_t cuid, Ptr<Socket> socket)
    : m_cybertwinId(cuid),
      m_localSocket(socket),
      m_globalSocket(nullptr)
{
    Address sockAddr;
    m_localSocket->GetSockName(sockAddr);
    if (InetSocketAddress::IsMatchingType(sockAddr))
    {
        m_address = InetSocketAddress::ConvertFrom(sockAddr).GetIpv4();
    }
    else if (Inet6SocketAddress::IsMatchingType(sockAddr))
    {
        m_address = Inet6SocketAddress::ConvertFrom(sockAddr).GetIpv6();
    }
    m_localSocket->SetRecvCallback(MakeCallback(&Cybertwin::RecvFromSocket, this));
    m_localSocket->SetCloseCallbacks(MakeCallback(&Cybertwin::LocalNormalCloseCallback, this),
                                     MakeCallback(&Cybertwin::LocalErrorCloseCallback, this));
}

Cybertwin::~Cybertwin()
{
}

void
Cybertwin::StartApplication()
{
    NS_LOG_FUNCTION(m_cybertwinId);
    NS_ASSERT_MSG(m_localSocket, "Connection socket not set");
    if (!m_globalSocket)
    {
        m_globalSocket =
            Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        m_port = DoSocketBind(m_globalSocket, m_address);
    }
    m_globalSocket->SetAcceptCallback(MakeCallback(&Cybertwin::GlobalConnRequestCallback, this),
                                      MakeCallback(&Cybertwin::GlobalConnCreatedCallback, this));
    m_globalSocket->SetCloseCallbacks(MakeCallback(&Cybertwin::GlobalNormalCloseCallback, this),
                                      MakeCallback(&Cybertwin::GlobalErrorCloseCallback, this));
    m_globalSocket->Listen();
    NS_LOG_DEBUG("--[Edge" << GetNode()->GetId() << "-#" << m_cybertwinId
                           << "]: starts listening globally at port " << m_port);
    // Temporary, simulate the initialization process
    Simulator::Schedule(Seconds(3.), &Cybertwin::Initialize, this);
}

void
Cybertwin::StopApplication()
{
    NS_LOG_FUNCTION(m_cybertwinId);
    for (auto it = m_streamBuffer.begin(); it != m_streamBuffer.end(); ++it)
    {
        it->first->Close();
    }
    m_streamBuffer.clear();
    if (m_localSocket)
    {
        m_localSocket->Close();
        m_localSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    if (m_globalSocket)
    {
        m_globalSocket->Close();
        m_globalSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

void
Cybertwin::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_localSocket = nullptr;
    m_globalSocket = nullptr;
    Application::DoDispose();
}

void
Cybertwin::Initialize()
{
    NS_LOG_FUNCTION(m_cybertwinId);
    // notify the client through the InitCallback that a cybertwin has been created successfully
    CybertwinHeader rspHeader;
    rspHeader.SetCybertwin(m_cybertwinId);
    rspHeader.SetCommand(CYBERTWIN_CONNECT_SUCCESS);

    Ptr<Packet> rspPacket = Create<Packet>(0);
    rspPacket->AddHeader(rspHeader);

    // simulate a simple GNRS
    if (Ipv4Address::IsMatchingType(m_address))
    {
        GlobalRouteTable[m_cybertwinId] =
            InetSocketAddress(Ipv4Address::ConvertFrom(m_address), m_port);
    }
    else
    {
        GlobalRouteTable[m_cybertwinId] =
            Inet6SocketAddress(Ipv6Address::ConvertFrom(m_address), m_port);
    }
    m_localSocket->Send(rspPacket);
}

void
Cybertwin::LocalNormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    socket->ShutdownSend();
    // client wants to close
    m_streamBuffer.erase(socket);
}

void
Cybertwin::LocalErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket->GetErrno());
}

bool
Cybertwin::GlobalConnRequestCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: received global connection request at "
                             << Simulator::Now());
    return true;
}

void
Cybertwin::GlobalConnCreatedCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this);
    socket->SetRecvCallback(MakeCallback(&Cybertwin::RecvFromSocket, this));
}

void
Cybertwin::GlobalNormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    socket->ShutdownSend();
    // client wants to close
    m_streamBuffer.erase(socket);
}

void
Cybertwin::GlobalErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket->GetErrno());
}

void
Cybertwin::RecvFromSocket(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            break;
        }

        Ptr<Packet> buffer;
        if (m_streamBuffer.find(socket) == m_streamBuffer.end())
        {
            m_streamBuffer[socket] = Create<Packet>(0);
        }
        buffer = m_streamBuffer[socket];
        buffer->AddAtEnd(packet);

        CybertwinHeader header;
        buffer->PeekHeader(header);
        NS_ASSERT_MSG(header.isDataPacket(),
                      "--[Edge-#" << m_cybertwinId << "]: received invalid packet type");

        while (buffer->GetSize() >= header.GetSize())
        {
            if (header.GetCommand() == HOST_SEND)
            {
                RecvLocalPacket(buffer->CreateFragment(0, header.GetSize()));
            }
            else if (header.GetCommand() == CYBERTWIN_SEND)
            {
                RecvGlobalPacket(buffer->CreateFragment(0, header.GetSize()));
            }
            else
            {
                NS_LOG_ERROR("UNKNOWN PACKET");
            }

            buffer->RemoveAtStart(header.GetSize());
            if (buffer->GetSize() > header.GetSerializedSize())
            {
                buffer->PeekHeader(header);
            }
            else
            {
                break;
            }
        }
    }
}

void
Cybertwin::RecvLocalPacket(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(m_cybertwinId << packet->ToString());
    CybertwinHeader recvHeader;
    packet->RemoveHeader(recvHeader);
    CYBERTWINID_t peerCuid = recvHeader.GetPeer();
    // TODO
    InetSocketAddress addr = InetSocketAddress::ConvertFrom(GlobalRouteTable[peerCuid]);
    if (m_txBuffer.find(peerCuid) == m_txBuffer.end())
    {
        Ptr<Socket> txSocket =
            Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        DoSocketBind(txSocket, m_address);
        txSocket->Connect(addr);
        m_txBuffer[peerCuid] = txSocket;
    }
    CybertwinHeader sendHeader(recvHeader);
    sendHeader.SetCommand(CYBERTWIN_SEND);
    sendHeader.SetPeer(recvHeader.GetCybertwin());
    sendHeader.SetCybertwin(recvHeader.GetPeer());
    sendHeader.SetCredit(10000);
    packet->AddHeader(sendHeader);
    m_txBuffer[peerCuid]->Send(packet);
}

void
Cybertwin::RecvGlobalPacket(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(m_cybertwinId << packet->ToString());
}

} // namespace ns3