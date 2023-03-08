#include "cybertwin.h"

#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Cybertwin");
NS_OBJECT_ENSURE_REGISTERED(Cybertwin);

TypeId
Cybertwin::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Cybertwin")
            .SetParent<Application>()
            .SetGroupName("cybertwin")
            .AddConstructor<Cybertwin>()
            .AddAttribute("LocalAddress",
                          "The address on which to bind the locally listening socket",
                          AddressValue(),
                          MakeAddressAccessor(&Cybertwin::m_localAddr),
                          MakeAddressChecker())
            .AddAttribute("LocalPort",
                          "The port on which the application sends data to host",
                          UintegerValue(443),
                          MakeUintegerAccessor(&Cybertwin::m_localPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("GlobalAddress",
                          "The address on which to bind the globally listening socket",
                          AddressValue(),
                          MakeAddressAccessor(&Cybertwin::m_globalAddr),
                          MakeAddressChecker())
            .AddAttribute("GlobalPort",
                          "The port on which the application sends data to the network",
                          UintegerValue(443),
                          MakeUintegerAccessor(&Cybertwin::m_globalPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("CybertwinId",
                          "The CUID of this cybertwin",
                          UintegerValue(),
                          MakeUintegerAccessor(&Cybertwin::m_cybertwinId),
                          MakeUintegerChecker<uint64_t>());
    return tid;
}

Cybertwin::Cybertwin()
    : m_localSocket(nullptr),
      m_globalSocket(nullptr)
{
    NS_LOG_FUNCTION(this);
}

Cybertwin::~Cybertwin()
{
    NS_LOG_FUNCTION(this);
}

void
Cybertwin::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_localSocket)
    {
        m_localSocket =
            Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        DoSocketMethod(&Socket::Bind, m_localSocket, m_localAddr, m_localPort);
    }

    m_localSocket->SetAcceptCallback(MakeCallback(&Cybertwin::LocalConnRequestCallback, this),
                                     MakeCallback(&Cybertwin::LocalConnCreatedCallback, this));
    m_localSocket->SetCloseCallbacks(MakeCallback(&Cybertwin::LocalNormalCloseCallback, this),
                                     MakeCallback(&Cybertwin::LocalErrorCloseCallback, this));
    m_localSocket->Listen();
    NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: "
                             << "Started Listening at " << Simulator::Now());
    // TODO: Global socket
    // Temporary, simulate the initialization process
    Simulator::Schedule(Seconds(1.), &Cybertwin::DoInit, this);
}

void
Cybertwin::StopApplication()
{
    NS_LOG_FUNCTION(this);
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
Cybertwin::DoInit()
{
    NS_LOG_FUNCTION(this);
    isInitialized = true;
}

void
Cybertwin::Setup(uint64_t cuid,
                 const Address& localAddr,
                 uint16_t localPort,
                 const Address& globalAddr,
                 uint16_t globalPort)
{
    m_cybertwinId = cuid;
    m_localAddr = localAddr;
    m_localPort = localPort;
    m_globalAddr = globalAddr;
    m_globalPort = globalPort;
}

bool
Cybertwin::LocalConnRequestCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Received connection request at " << Simulator::Now());
    return isInitialized;
}

void
Cybertwin::LocalConnCreatedCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this);
    socket->SetRecvCallback(MakeCallback(&Cybertwin::RecvFromLocal, this));
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

void
Cybertwin::RecvFromLocal(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            break;
        }

        Ptr<Packet> buffer;
        CybertwinPacketHeader header;

        if (m_streamBuffer.find(socket) == m_streamBuffer.end())
        {
            m_streamBuffer[socket] = Create<Packet>(0);
        }

        buffer = m_streamBuffer[socket];
        buffer->AddAtEnd(packet);
        buffer->PeekHeader(header);

        NS_ABORT_IF(header.GetSize() == 0);

        while (buffer->GetSize() >= header.GetSize())
        {
            ReceivePacket(buffer->CreateFragment(0, header.GetSize()));
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
Cybertwin::ReceivePacket(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
    NS_LOG_DEBUG(packet->ToString());
}

} // namespace ns3