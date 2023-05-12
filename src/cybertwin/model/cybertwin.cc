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

    // init multipath log file
    m_MpLogFileName = "mp-test-" + std::to_string(cuid) + ".log";
    m_MpLogFile.open(m_MpLogFileName, std::ios::out);
    m_MpLogFile.close();
}



Cybertwin::~Cybertwin()
{
}

void
Cybertwin::StartApplication()
{
    NS_LOG_FUNCTION(m_cybertwinId);
    m_cnrs = GetNode()->GetObject<CybertwinEdgeServer>()->GetCNRSApp();

#if MDTP_ENABLED
    // init data server and listen for incoming connections
    m_dtServer = new CybertwinDataTransferServer();
    m_dtServer->Setup(GetNode(), m_cybertwinId, m_interfaces);
    m_dtServer->Listen();
    m_dtServer->SetNewConnectCreatedCallback(
        MakeCallback(&Cybertwin::NewMpConnectionCreatedCallback, this));
#else
    // init data server and listen for incoming connections
    m_dtServer = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
    //FIXME: attention, the port number is hard-coded here
    m_dtServer->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_interfaces[0].second));
    m_dtServer->SetAcceptCallback(MakeCallback(&Cybertwin::NewSpConnectionRequestCallback, this),
                                  MakeCallback(&Cybertwin::NewSpConnectionCreatedCallback, this));
    m_dtServer->Listen();
#endif

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

    for (uint32_t i=0; i<CORE_CLOUD_REPLICATION_RATIO; i++)
    {
        Ptr<Packet> pack = packet->Copy();
        m_txPendingBuffer[peerCuid].push(pack);
    }

    // Use Simulator::ScheduleNow to avoid long execution time for a single event
    Simulator::ScheduleNow(&Cybertwin::SendPendingPackets, this, peerCuid);
}

void
Cybertwin::SendPendingPackets(CYBERTWINID_t peerCuid)
{
    // get connection by peer cuid
#if MDTP_ENABLED
    MultipathConnection* conn = nullptr;
#else
    Ptr<Socket> conn = nullptr;
#endif

    if (m_txConnections.find(peerCuid) != m_txConnections.end())
    {
    // case[1]: connection is established, output packets
        conn = m_txConnections[peerCuid];
        // TODO: limit the number of packets to send
        while (!m_txPendingBuffer[peerCuid].empty())
        {
            Ptr<Packet> packet = m_txPendingBuffer[peerCuid].front();
            m_txPendingBuffer[peerCuid].pop();
            SendPacket(peerCuid, conn, packet);
        }
    }
    else if (m_pendingConnections.find(peerCuid) != m_pendingConnections.end())
    {
    // case[2]: connection is establishing, wait for establishment
        NS_LOG_DEBUG("--[Edge-#"
                     << m_cybertwinId
                     << "]: connection is establishing, wait for the connection to be created at "
                     << Simulator::Now());
    }
    else
    {
    // case[3]: connection not created yet, request to create.
        NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId
                                 << "]: connection not created yet, initiate a new connection at "
                                 << Simulator::Now());
        // connection not created yet, initiate a new connection
#if MDTP_ENABLED
        conn = new MultipathConnection();
        conn->Setup(GetNode(), m_cybertwinId, m_interfaces);
        conn->Connect(peerCuid);
        // send pending packets by callback after connection is created
        conn->SetConnectCallback(MakeCallback(&Cybertwin::NewMpConnectionCreatedCallback, this),
                                 MakeCallback(&Cybertwin::NewMpConnectionErrorCallback, this));
#else
        conn = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        conn->Bind();
        m_cnrs->GetCybertwinInterfaceByName(peerCuid,
                                            MakeCallback(&Cybertwin::SocketConnectWithResolvedCybertwinName, this, conn));
        conn->SetConnectCallback(MakeCallback(&Cybertwin::NewSpConnectionCreatedCallback, this),
                                 MakeCallback(&Cybertwin::NewSpConnectionErrorCallback, this));
        m_pendingConnectionsReverse[conn] = peerCuid;
#endif

        // insert to pending set
        m_pendingConnections[peerCuid] = conn;
    }
}

void
Cybertwin::SocketConnectWithResolvedCybertwinName(Ptr<Socket> socket, CYBERTWINID_t cuid, CYBERTWIN_INTERFACE_LIST_t ifs)
{
    NS_LOG_FUNCTION(this << socket << cuid);
    NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: connect to " << cuid << "using socket at " << Simulator::Now());
    if (ifs.size() == 0)
    {
        NS_LOG_ERROR("No interface found for " << cuid);
        return;
    }
    //TODO: a reasonable way to select an interface
    InetSocketAddress peeraddr = InetSocketAddress(ifs.at(0).first, ifs.at(0).second);
    socket->Connect(peeraddr);
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

void
Cybertwin::CybertwinServerBulkSend(
#if MDTP_ENABLED
    MultipathConnection* conn
#else
    Ptr<Socket> conn
#endif
)
{
    NS_ASSERT_MSG(conn, "Connection is null");

    if (m_serverTxBytes < 100 * 1024 * 1024) //100GB
    {
        Ptr<Packet> packet = Create<Packet>(516);
        conn->Send(packet);
        m_serverTxBytes++;

        // 516 bytes / 1000 ns = 516MBps
        Simulator::Schedule(MicroSeconds(1),
                            &Cybertwin::CybertwinServerBulkSend,
                            this,
                            conn);
    }
}

void
Cybertwin::UpdateRxSizePerSecond(
#if MDTP_ENABLED
    CYBERTWINID_t id
#else
    Ptr<Socket> id
#endif
)
{
    NS_LOG_FUNCTION(this << id);
    if (m_rxSizePerSecond.find(id) == m_rxSizePerSecond.end())
    {
        NS_FATAL_ERROR("logic error. Please check here");
    }

    uint64_t recved = m_rxSizePerSecond[id];
    m_rxSizePerSecond[id] = 0;

    NS_LOG_UNCOND("Cybertwin[" << m_cybertwinId << "]: received " << recved << " bytes in last 10ms");
    if (!m_MpLogFile.is_open())
    {
        m_MpLogFile.open(m_MpLogFileName, std::ios::out | std::ios::app);
        NS_ASSERT_MSG(m_MpLogFile.is_open(), "Open log file failed.");
    }
    m_MpLogFile<<Simulator::Now().GetSeconds()<<" "<<recved<<std::endl;
    m_MpLogFile.close();

    Simulator::Schedule(MilliSeconds(STATISTIC_TIME_INTERVAL), &Cybertwin::UpdateRxSizePerSecond, this, id);
}

#if MDTP_ENABLED
// Multipath connection Callbacks
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
    if (m_pendingConnections.find(conn->m_peerCyberID) != m_pendingConnections.end())
    {
        // case1: txPendingBuffer not empty means this cybertwin have initiated a connection request
        NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: connection with " << conn->m_peerCyberID
                                  << " is successfully established");

        // erase the connection from pendingConnections, and insert it to txConnections
        m_pendingConnections.erase(conn->m_peerCyberID);
        m_txConnections[conn->m_peerCyberID] = conn;

        // after connection created, we schedule a send event to send pending packets
        Simulator::ScheduleNow(&Cybertwin::SendPendingPackets, this, conn->m_peerCyberID);
        Simulator::Schedule(MilliSeconds(STATISTIC_TIME_INTERVAL), &Cybertwin::UpdateRxSizePerSecond, this, conn->m_peerCyberID);
    }
    else
    {
        // case2: conn not found in txConnections means DataServer received a connection request and
        //  successfully created a connection
        NS_LOG_DEBUG(
            "Cybertwin[" << m_cybertwinId << "]: DataServer received a connection request from "
                         << conn->m_peerCyberID << " and successfully created a connection");
        m_rxConnections[conn->m_peerCyberID] = conn;
        m_rxSizePerSecond[conn->m_peerCyberID] = 0;
        Simulator::Schedule(MilliSeconds(STATISTIC_TIME_INTERVAL), &Cybertwin::UpdateRxSizePerSecond, this, conn->m_peerCyberID);
        Simulator::ScheduleNow(&Cybertwin::CybertwinServerBulkSend, this, conn);
    }

    conn->SetRecvCallback(MakeCallback(&Cybertwin::MpConnectionRecvCallback, this));
    conn->SetCloseCallback(MakeCallback(&Cybertwin::MpConnectionClosedCallback, this));
}

void
Cybertwin:: MpConnectionRecvCallback(MultipathConnection* conn)
{
    NS_ASSERT_MSG(conn != nullptr, "Connection is null");
    Ptr<Packet> packet;
    while ((packet = conn->Recv()))
    {
        NS_LOG_INFO("--[Edge" << GetNode()->GetId() << "-#" << m_cybertwinId
                               << "]: received packet from " << conn->m_peerCyberID << " with size "
                               << packet->GetSize() << " bytes");
        // TODO: what to do next? Send to client or save to buffer.
        m_rxSizePerSecond[conn->m_peerCyberID] += packet->GetSize();
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

void
Cybertwin::DownloadFileServer(MultipathConnection* conn)
{
    NS_LOG_FUNCTION(this << conn->m_peerCyberID);


}

#else
// Naive Socket Callbacks
void
Cybertwin::NewSpConnectionErrorCallback(Ptr<Socket> sock)
{
    Address peerAddr;
    sock->GetPeerName(peerAddr);
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: connection error with "
                              << peerAddr);
    // TODO: failed to create a connection, how to handle the pending data?
    // one way is to set a timer to retry or discard the pending data
}

bool
Cybertwin::NewSpConnectionRequestCallback(Ptr<Socket> sock)
{
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: received a connection request");
    return true;
}

void
Cybertwin::NewSpConnectionCreatedCallback(Ptr<Socket> sock)
{
    NS_ASSERT_MSG(sock != nullptr, "Connection is null");
    if (m_pendingConnectionsReverse.find(sock) != m_pendingConnectionsReverse.end())
    {
    // case[1]: socket find in tx pending connections means this cybertwin have 
    //          initiated a connection, and now it is successfully established
        CYBERTWINID_t peerid = m_pendingConnectionsReverse[sock];
        NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: connection with " << peerid
                                  << " is successfully established");

        // erase the connection from pendingConnections, and insert it to txConnections
        m_pendingConnections.erase(peerid);
        m_pendingConnectionsReverse.erase(sock);
        m_txConnections[peerid] = sock;
        m_txConnectionsReverse[sock] = peerid;

        // after connection created, we schedule a send event to send pending packets
        Simulator::ScheduleNow(&Cybertwin::SendPendingPackets, this, peerid);
        Simulator::Schedule(MilliSeconds(STATISTIC_TIME_INTERVAL), &Cybertwin::UpdateRxSizePerSecond, this, sock);
    }
    else
    {
    // case[2]: socket not found in pending connections means DataServer received a
    //          request and successfully created a connection
        Address peeraddr;
        sock->GetPeerName(peeraddr);

        NS_LOG_DEBUG(
            "Cybertwin[" << m_cybertwinId << "]: DataServer received a connection request from "
                         <<peeraddr  << " and successfully created a connection");
        m_rxConnections.insert(sock);
        m_rxConnectionsReverse[sock] = peeraddr;
        m_rxSizePerSecond[sock] = 0;

        Simulator::Schedule(MilliSeconds(STATISTIC_TIME_INTERVAL), &Cybertwin::UpdateRxSizePerSecond, this, sock);
        Simulator::ScheduleNow(&Cybertwin::CybertwinServerBulkSend, this, sock);
    }

    sock->SetRecvCallback(MakeCallback(&Cybertwin::SpConnectionRecvCallback, this));
    sock->SetCloseCallbacks(MakeCallback(&Cybertwin::SpNormalCloseCallback, this),
                            MakeCallback(&Cybertwin::SpErrorCloseCallback, this));
}

void
Cybertwin::SpConnectionRecvCallback(Ptr<Socket> sock)
{
    Ptr<Packet> packet;
    while ((packet = sock->Recv()))
    {
        NS_LOG_INFO("--[Edge" << GetNode()->GetId() << "-#" << m_cybertwinId
                               << "]: received packet from " << m_rxConnectionsReverse[sock] << " with size "
                               << packet->GetSize() << " bytes");
        // TODO: what to do next? Send to client or save to buffer.
        m_rxSizePerSecond[sock] += packet->GetSize();
    }
}

void
Cybertwin::SpNormalCloseCallback(Ptr<Socket> sock)
{
    Address peeraddr;
    sock->GetPeerName(peeraddr);
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: connection closed with "
                              << peeraddr);
    if (m_txConnectionsReverse.find(sock) != m_txConnectionsReverse.end())
    {
        m_txConnectionsReverse.erase(sock);
    }
    else if (m_rxConnectionsReverse.find(sock) != m_rxConnectionsReverse.end())
    {
        m_rxConnectionsReverse.erase(sock);
        m_rxConnections.erase(sock);
    }
    else
    {
        NS_LOG_ERROR("Cybertwin[" << m_cybertwinId << "]: connection closed with "
                                  << peeraddr << " but no such connection found");
    }
}

void
Cybertwin::SpErrorCloseCallback(Ptr<Socket> sock)
{
    Address peeraddr;
    sock->GetPeerName(peeraddr); 
    NS_LOG_WARN("Cybertwin[" << m_cybertwinId << "]: connection error with "
                             << peeraddr);

}

#endif

} // namespace ns3
