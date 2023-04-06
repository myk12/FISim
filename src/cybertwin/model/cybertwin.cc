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
      m_localPort(0),
      m_globalSocket(nullptr),
      m_globalPort(0)
{
}

Cybertwin::Cybertwin(CYBERTWINID_t cuid,
                     const Address& address,
                     CybertwinInitCallback initCallback,
                     CybertwinSendCallback sendCallback,
                     CybertwinReceiveCallback receiveCallback)
    : InitCybertwin(initCallback),
      SendPacket(sendCallback),
      ReceivePacket(receiveCallback),
      m_cybertwinId(cuid),
      m_address(address)
{
}

Cybertwin::~Cybertwin()
{
}

void
Cybertwin::StartApplication()
{
    NS_LOG_FUNCTION(m_cybertwinId);
    m_localSocket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
    m_localPort = DoSocketBind(m_localSocket, m_address);
    m_localSocket->SetAcceptCallback(MakeCallback(&Cybertwin::LocalConnRequestCallback, this),
                                     MakeCallback(&Cybertwin::LocalConnCreatedCallback, this));
    m_localSocket->SetCloseCallbacks(MakeCallback(&Cybertwin::LocalNormalCloseCallback, this),
                                     MakeCallback(&Cybertwin::LocalErrorCloseCallback, this));
    m_localSocket->Listen();

    m_globalSocket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
    m_globalPort = DoSocketBind(m_globalSocket, m_address);
    m_globalSocket->SetAcceptCallback(MakeCallback(&Cybertwin::GlobalConnRequestCallback, this),
                                      MakeCallback(&Cybertwin::GlobalConnCreatedCallback, this));
    m_globalSocket->SetCloseCallbacks(MakeCallback(&Cybertwin::GlobalNormalCloseCallback, this),
                                      MakeCallback(&Cybertwin::GlobalErrorCloseCallback, this));
    m_globalSocket->Listen();

    NS_LOG_DEBUG("--[Edge" << GetNode()->GetId() << "-#" << m_cybertwinId
                           << "]: starts listening locally at port " << m_localPort
                           << " and globally at port " << m_globalPort);
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
    rspHeader.SetCybertwinPort(m_localPort);

    // simulate a simple GNRS
    if (Ipv4Address::IsMatchingType(m_address))
    {
        GlobalRouteTable[m_cybertwinId] =
            InetSocketAddress(Ipv4Address::ConvertFrom(m_address), m_globalPort);
    }
    else
    {
        GlobalRouteTable[m_cybertwinId] =
            Inet6SocketAddress(Ipv6Address::ConvertFrom(m_address), m_globalPort);
    }

    InitCybertwin(rspHeader);
}

bool
Cybertwin::LocalConnRequestCallback(Ptr<Socket> socket, const Address&)
{
    return true;
}

void
Cybertwin::LocalConnCreatedCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this);
    socket->SetRecvCallback(MakeCallback(&Cybertwin::RecvFromSocket, this));
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
        NS_ASSERT(header.isDataPacket());
        while (buffer->GetSize() >= header.GetSize())
        {
            if (header.GetCybertwin() == m_cybertwinId)
            {
                // packet from host
                RecvLocalPacket(header, buffer->CreateFragment(0, header.GetSize()));
            }
            else if (header.GetPeer() == m_cybertwinId)
            {
                // packet from cybertwin
                RecvGlobalPacket(header, buffer->CreateFragment(0, header.GetSize()));
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
Cybertwin::RecvLocalPacket(const CybertwinHeader& header, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(m_cybertwinId << packet->ToString());
    CYBERTWINID_t peerCuid = header.GetPeer();
    // TODO
    InetSocketAddress addr = InetSocketAddress::ConvertFrom(GlobalRouteTable[peerCuid]);
    if (m_txBuffer.find(peerCuid) == m_txBuffer.end())
    {
        Ptr<Socket> txSocket =
            Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        DoSocketBind(txSocket, m_address);
        txSocket->Connect(addr);
        // TODO: Forward only after connected
        m_txBuffer[peerCuid] = txSocket;
    }
    SendPacket(peerCuid, m_txBuffer[peerCuid], packet);
}

void
Cybertwin::RecvGlobalPacket(const CybertwinHeader& header, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(m_cybertwinId << packet->ToString());
    // TODO
}

} // namespace ns3