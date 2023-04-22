#include "cybertwin.h"

#include "cybertwin-name-resolution-service.h"

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
      m_localPort(0)
{
}

Cybertwin::Cybertwin(CYBERTWINID_t cuid,
                     CYBERTWIN_INTERFACE_LIST_t g_interfaces,
                     const Address& address,
                     CybertwinInitCallback initCallback,
                     CybertwinSendCallback sendCallback,
                     CybertwinReceiveCallback receiveCallback)
    : InitCybertwin(initCallback),
      SendPacket(sendCallback),
      ReceivePacket(receiveCallback),
      m_cybertwinId(cuid),
      m_address(address),
      m_interfaces(g_interfaces)
{
    NS_LOG_FUNCTION(cuid);
}

void
Cybertwin::NewMpConnectionErrorCallback(MultipathConnection* conn)
{
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: connection error with "
                              << conn->m_peerCyberID);
    // TODO: failed to create a connection, how to handle the pending data?
}

void
Cybertwin::NewMpConnectionCreatedCallback(MultipathConnection* conn)
{
    NS_LOG_DEBUG("New connection created: " << conn->GetConnID());
    NS_ASSERT_MSG(conn != nullptr, "Connection is null");
    if (m_txPendingBuffer.find(conn->m_peerCyberID) != m_txPendingBuffer.end())
    {
        // case1: txPendingBuffer not empty means this cybertwin have initiated a connection request
        NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: connection with " << conn->m_peerCyberID
                                  << " is successfully established");

        // erase the connection from pendingConnections, and insert it to txConnections
        m_pendingConnections.erase(conn->m_peerCyberID);
        m_txConnections[conn->m_peerCyberID] = conn;

        // after connection created, we schedule a send event to send pending packets
        Simulator::ScheduleNow(&Cybertwin::SendPendingPackets, this, conn->m_peerCyberID);
    }
    else
    {
        // case2: conn not found in txConnections means DataServer received a connection request and
        //  successfully created a connection
        NS_LOG_DEBUG(
            "Cybertwin[" << m_cybertwinId << "]: DataServer received a connection request from "
                         << conn->m_peerCyberID << " and successfully created a connection");
        m_rxConnections[conn->m_peerCyberID] = conn;
    }

    conn->SetRecvCallback(MakeCallback(&Cybertwin::MpConnectionRecvCallback, this));
    conn->SetCloseCallback(MakeCallback(&Cybertwin::MpConnectionClosedCallback, this));
}

void
Cybertwin::MpConnectionRecvCallback(MultipathConnection* conn)
{
    NS_ASSERT_MSG(conn != nullptr, "Connection is null");
    Ptr<Packet> packet;
    while ((packet = conn->Recv()))
    {
        NS_LOG_DEBUG("--[Edge" << GetNode()->GetId() << "-#" << m_cybertwinId
                               << "]: received packet from " << conn->m_peerCyberID << " with size "
                               << packet->GetSize() << " bytes");
        // TODO: what to do next? Send to client or save to buffer.
    }
}

void
Cybertwin::MpConnectionClosedCallback(MultipathConnection* conn)
{
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: connection closed with "
                              << conn->m_peerCyberID);
    if (m_txConnections.find(conn->m_peerCyberID) != m_txConnections.end())
    {
        m_txConnections.erase(conn->m_peerCyberID);
    }
    else if (m_rxConnections.find(conn->m_peerCyberID) != m_rxConnections.end())
    {
        m_rxConnections.erase(conn->m_peerCyberID);
    }
    else
    {
        NS_LOG_ERROR("Cybertwin[" << m_cybertwinId << "]: connection closed with "
                                  << conn->m_peerCyberID << " but no such connection found");
    }
}

Cybertwin::~Cybertwin()
{
}

void
Cybertwin::StartApplication()
{
    NS_LOG_FUNCTION(m_cybertwinId);
    m_cnrs = GetNode()->GetObject<CybertwinEdgeServer>()->GetCNRSApp();

    // init data server and listen for incoming connections
    m_dtServer = new CybertwinDataTransferServer();
    m_dtServer->Setup(GetNode(), m_cybertwinId, m_interfaces);
    m_dtServer->Listen();
    m_dtServer->SetNewConnectCreatedCallback(
        MakeCallback(&Cybertwin::NewMpConnectionCreatedCallback, this));

    // report interfaces to CNRS
    m_cnrs->InsertCybertwinInterfaceName(m_cybertwinId, m_interfaces);

    m_localSocket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
    m_localPort = DoSocketBind(m_localSocket, m_address);
    m_localSocket->SetAcceptCallback(MakeCallback(&Cybertwin::LocalConnRequestCallback, this),
                                     MakeCallback(&Cybertwin::LocalConnCreatedCallback, this));
    m_localSocket->SetCloseCallbacks(MakeCallback(&Cybertwin::LocalNormalCloseCallback, this),
                                     MakeCallback(&Cybertwin::LocalErrorCloseCallback, this));
    m_localSocket->Listen();

    NS_LOG_DEBUG("--[Edge" << GetNode()->GetId() << "-#" << m_cybertwinId
                           << "]: starts listening locally at port " << m_localPort);
    // Temporary, simulate the initialization process
    Simulator::ScheduleNow(&Cybertwin::Initialize, this);
    // Simulator::Schedule(Seconds(3.), &Cybertwin::Initialize, this);
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
}

void
Cybertwin::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_localSocket = nullptr;
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

#if 0 // Disable GlobalRouteTable
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
#endif

    Simulator::ScheduleNow(&Cybertwin::InitCybertwin, this, rspHeader);
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
    // NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: received local packet at "
    //                          << Simulator::Now());
    CYBERTWINID_t peerCuid = header.GetPeer();

    // save to pending buffer
    if (m_txPendingBuffer.find(peerCuid) == m_txPendingBuffer.end())
    {
        m_txPendingBuffer[peerCuid] = std::queue<Ptr<Packet>>();
    }
    m_txPendingBuffer[peerCuid].push(packet);

    // Use Simulator::ScheduleNow to avoid long execution time for a single event
    Simulator::ScheduleNow(&Cybertwin::SendPendingPackets, this, peerCuid);
}

void
Cybertwin::SendPendingPackets(CYBERTWINID_t peerCuid)
{
    // NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: send pending packets at " <<
    // Simulator::Now());
    // get connection by peer cuid
    MultipathConnection* conn = nullptr;
    if (m_txConnections.find(peerCuid) != m_txConnections.end())
    {
        // case1: connection established
        conn = m_txConnections[peerCuid];
        // TODO: limit the number of packets to send
        while (!m_txPendingBuffer[peerCuid].empty())
        {
            Ptr<Packet> packet = m_txPendingBuffer[peerCuid].front();
            m_txPendingBuffer[peerCuid].pop();
            SendPacket(peerCuid, conn, packet);
            // conn->Send(packet);
        }
    }
    else if (m_pendingConnections.find(peerCuid) != m_pendingConnections.end())
    {
        // case1: connection is establishing
        NS_LOG_DEBUG("--[Edge-#"
                     << m_cybertwinId
                     << "]: connection is establishing, wait for the connection to be created at "
                     << Simulator::Now());
    }
    else
    {
        // case2: connection not created yet
        NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId
                                 << "]: connection not created yet, initiate a new connection at "
                                 << Simulator::Now());
        // connection not created yet, initiate a new connection
        conn = new MultipathConnection();
        conn->Setup(GetNode(), m_cybertwinId);
        conn->Connect(peerCuid);

        // insert to pending set
        m_pendingConnections[peerCuid] = conn;

        // set the callback
        // send pending packets by callback after connection is created
        conn->SetConnectCallback(MakeCallback(&Cybertwin::NewMpConnectionCreatedCallback, this),
                                 MakeCallback(&Cybertwin::NewMpConnectionErrorCallback, this));
    }
}

void
Cybertwin::RecvGlobalPacket(const CybertwinHeader& header, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(m_cybertwinId << packet->ToString());
    // TODO
}

void
Cybertwin::ForwardLocalPacket(CYBERTWINID_t cuid, CYBERTWIN_INTERFACE_LIST_t& ifs)
{
    NS_LOG_FUNCTION(cuid);
    for (auto interface : ifs)
    {
        NS_LOG_DEBUG(interface.first << interface.second);
    }
}

} // namespace ns3
