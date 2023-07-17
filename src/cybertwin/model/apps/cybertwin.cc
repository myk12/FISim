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
      m_localSocket(nullptr)
{
}

Cybertwin::Cybertwin(CYBERTWINID_t cuid,
                     CYBERTWIN_INTERFACE_t l_interface,
                     CYBERTWIN_INTERFACE_LIST_t g_interfaces)
    : m_cybertwinId(cuid),
      m_localSocket(nullptr),
      m_localInterface(l_interface),
      m_globalInterfaces(g_interfaces),
      m_isStart(false),
      m_comm_test_total_bytes(0),
      m_comm_test_interval_bytes(0)
{
    NS_LOG_FUNCTION(cuid);
}

Cybertwin::~Cybertwin()
{
}

void
Cybertwin::StartApplication()
{
    NS_LOG_FUNCTION("Cybertwin[" << m_cybertwinId << "] Start up.");

    // start listen at local port
    Simulator::ScheduleNow(&Cybertwin::LocallyListen, this);

    // start forwarding local packets
    //Simulator::ScheduleNow(&Cybertwin::LocallyForward, this);

    // start listen at global ports
    Simulator::ScheduleNow(&Cybertwin::GloballyListen, this);

    // report interfaces to CNRS
    m_cnrs = DynamicCast<CybertwinNode>(GetNode())->GetCNRSApp();
    NS_ASSERT(m_cnrs != nullptr);
    m_cnrs->InsertCybertwinInterfaceName(m_cybertwinId, m_globalInterfaces);

    // open log
    OpenLogFile(DynamicCast<CybertwinNode>(GetNode())->GetLogDir(), "cybertwin.log");
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

//***************************************************************************************
//*                     Handle incoming connections from host                           *
//***************************************************************************************

void
Cybertwin::LocallyListen()
{
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: starts listening locally at "
                              << m_localInterface.first << ":" << m_localInterface.second);
    if (!m_localSocket)
    {
        m_localSocket =
            Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        NS_ASSERT(m_localSocket != nullptr);
    }

    InetSocketAddress localAddr =
        InetSocketAddress(m_localInterface.first, m_localInterface.second);
    if (m_localSocket->Bind(localAddr) < 0)
    {
        NS_LOG_ERROR("Failed to bind local socket to " << localAddr);
        return;
    }

    m_localSocket->SetAcceptCallback(MakeCallback(&Cybertwin::LocalConnRequestCallback, this),
                                     MakeCallback(&Cybertwin::LocalConnCreatedCallback, this));
    m_localSocket->SetCloseCallbacks(MakeCallback(&Cybertwin::LocalNormalCloseCallback, this),
                                     MakeCallback(&Cybertwin::LocalErrorCloseCallback, this));
    m_localSocket->Listen();
}

bool
Cybertwin::LocalConnRequestCallback(Ptr<Socket> socket, const Address&)
{
    NS_LOG_FUNCTION(this);
    return true;
}

void
Cybertwin::LocalConnCreatedCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this);
    socket->SetRecvCallback(MakeCallback(&Cybertwin::LocalRecvCallback, this));
}

void
Cybertwin::LocalNormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    socket->ShutdownSend();
}

void
Cybertwin::LocalErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket->GetErrno());
}

void
Cybertwin::LocalRecvCallback(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: receive empty packet, close socket");
            break;
        }

        Address localAddr, remoteAddr;
        socket->GetSockName(localAddr);
        socket->GetPeerName(remoteAddr);

        // get stream id
        CybertwinHeader header;
        packet->RemoveHeader(header);
        CYBERTWINID_t sender = header.GetCybertwin();
        CYBERTWINID_t receiver = header.GetPeer();

        if (receiver == COMM_TEST_CYBERTWIN_ID && sender == COMM_TEST_CYBERTWIN_ID)
        {
            // only used for comm test
            int32_t recvSize = packet->GetSize();
            Time m_currTime = Simulator::Now();
            m_comm_test_total_bytes +=  recvSize;
            m_comm_test_interval_bytes += recvSize;

            // put into queue
            m_tsPktQueue.push(packet);

            if (m_isStart == false)
            {
                // first packet, start statistics
                NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: start statistics");
                m_logStream << "--[Edge-#" << m_cybertwinId << "]: start statistics" << std::endl;
                m_isStart = true;
                m_startTime = m_currTime;
                m_lastTime = m_currTime;
                m_lastShapingTime = m_currTime;

                m_statisticalEnd = false;

                Simulator::ScheduleNow(&Cybertwin::CybertwinCommModelStatistical, this);
                Simulator::ScheduleNow(&Cybertwin::CybertwinCommModelTrafficShaping, this, 90);
            }
            
            continue;
        }

        STREAMID_t streamId = sender;
        streamId = streamId << 64 | receiver;

        NS_LOG_INFO("--[Edge-#" << m_cybertwinId << "]: received packet from " << sender << " to "
                                 << receiver << " with size " << packet->GetSize() << " bytes");

        if (m_txStreamBuffer.find(streamId) == m_txStreamBuffer.end())
        {
            NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId
                                     << "]: stream buffer not found, create a new one");
            m_txStreamBuffer[streamId] = std::queue<Ptr<Packet>>();
            m_txStreamBufferOrder.push_back(streamId);
        }

        // put into buffer
        std::queue<Ptr<Packet>>& buffer = m_txStreamBuffer[streamId];
        buffer.push(packet);
        if (buffer.size() > MAX_BUFFER_PKT_NUM)
        {
            NS_LOG_WARN("Stream buffer is full, drop the oldest packet");
            buffer.pop();
        }
        Simulator::ScheduleNow(&Cybertwin::SendPendingPackets, this, streamId);
    }
}

void
Cybertwin::LocallyForward()
{
    NS_LOG_FUNCTION(this);
    m_lastTxStreamBufferOrder = 0;

    if (m_txStreamBufferOrder.size() == 0)
    {
        // no stream buffer, wait for 10ms
        if (m_noneDataCnt++ < CYBERTWIN_FORWARD_MAX_WAIT_NUM)
        {
            Simulator::Schedule(MilliSeconds(10), &Cybertwin::LocallyForward, this);
        }
        return;
    }

    // pick a stream
    int32_t streamIdx = -1;
    int32_t idx = m_lastTxStreamBufferOrder;
    for (uint32_t i = 0; i < m_txStreamBufferOrder.size(); i++)
    {
        if (m_txStreamBuffer[idx].size() > 0)
        {
            streamIdx = idx;
            break;
        }
        idx = (idx + 1) % m_txStreamBufferOrder.size();
    }

    if (streamIdx == -1)
    {
        // no packets in all stream buffers, wait for 1ms
        Simulator::Schedule(MilliSeconds(1), &Cybertwin::LocallyForward, this);
        return;
    }

    // schedule to send packets
    NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: schedule to send packets in stream "
                             << streamIdx << " at " << Simulator::Now());
    STREAMID_t streamId = m_txStreamBufferOrder[streamIdx];
    Simulator::ScheduleNow(&Cybertwin::SendPendingPackets, this, streamId);
    //Simulator::Schedule(MilliSeconds(10), &Cybertwin::LocallyForward, this);
}

void
Cybertwin::SendPendingPackets(STREAMID_t streamId)
{
    // get connection by peer cuid
#if MDTP_ENABLED
    MultipathConnection* conn = nullptr;
#else
    Ptr<Socket> conn = nullptr;
#endif

    if (m_txConnections.find(streamId) != m_txConnections.end())
    {
        NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId
                                 << "]: connection is established, output packets");
        // case[1]: connection is established, output packets
        conn = m_txConnections[streamId];

        // TODO: limit the number of packets to send
        while (!m_txStreamBuffer[streamId].empty())
        {
            NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: send packet at " << Simulator::Now());
            Ptr<Packet> packet = m_txStreamBuffer[streamId].front();
            m_txStreamBuffer[streamId].pop();
            conn->Send(packet);
        }
    }
    else if (m_pendingConnections.find(streamId) != m_pendingConnections.end())
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
        conn->Setup(GetNode(), m_cybertwinId, m_globalInterfaces);
        conn->Connect(peerCuid);
        // send pending packets by callback after connection is created
        conn->SetConnectCallback(MakeCallback(&Cybertwin::NewMpConnectionCreatedCallback, this),
                                 MakeCallback(&Cybertwin::NewMpConnectionErrorCallback, this));
#else
        conn = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        conn->Bind();
        m_cnrs->GetCybertwinInterfaceByName(GET_PEERID_FROM_STREAMID(streamId),
                                            MakeCallback(&Cybertwin::SocketConnectWithResolvedCybertwinName, this, conn));
        
        m_pendingConnectionsReverse[conn] = streamId;
#endif

        // insert to pending set
        m_pendingConnections[streamId] = conn;
    }
}

void
Cybertwin::SocketConnectWithResolvedCybertwinName(Ptr<Socket> socket,
                                                  CYBERTWINID_t cuid,
                                                  CYBERTWIN_INTERFACE_LIST_t ifs)
{
    NS_LOG_FUNCTION(this << socket << cuid);
    if (ifs.size() == 0)
    {
        NS_LOG_ERROR("[Cybertwin] No interface found for " << cuid);
        return;
    }

    // TODO: a reasonable way to select an interface
    InetSocketAddress peeraddr = InetSocketAddress(ifs.at(0).first, ifs.at(0).second);
    NS_LOG_DEBUG("--[Edge-#" << m_cybertwinId << "]: connecting to " << ifs.at(0).first << ":" << ifs.at(0).second);
    socket->SetConnectCallback(MakeCallback(&Cybertwin::NewSpConnectionCreatedCallback, this),
                                MakeCallback(&Cybertwin::NewSpConnectionErrorCallback, this));
    socket->SetRecvCallback(MakeCallback(&Cybertwin::SpConnectionRecvCallback, this));
    if(socket->Connect(peeraddr) < 0)
    {
        NS_LOG_ERROR("[Cybertwin] Failed to connect to " << peeraddr << " with error " << socket->GetErrno());
    }
}

void
Cybertwin::CybertwinCommModelStatistical()
{
    NS_LOG_FUNCTION(this);
    uint64_t intervalBytes = m_comm_test_interval_bytes;
    if ((Simulator::Now() - m_startTime).GetSeconds() >= END_HOST_BULK_SEND_TEST_TIME)
    {
        // stop test
        NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: stop test");
        m_logStream << "Cybertwin[" << m_cybertwinId << "]: stop test" << std::endl;
        m_statisticalEnd = true;
        return;
    }

    m_comm_test_interval_bytes = 0;

    Time now = Simulator::Now();
    Time interval = now - m_lastTime;
    m_lastTime = now;

    double speed = intervalBytes * 8.0 / interval.GetSeconds() / 1000000.0; // Mbps

    // report thoughput
    m_logStream << "Cybertwin[" << m_cybertwinId << "]: at " << (now - m_startTime).GetMilliSeconds()
                << " ms, throughput = " << speed << " Mbps" << std::endl;
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: at " << (now - m_startTime).GetMilliSeconds()
                              << " ms, throughput = " << speed << " Mbps");

    // schedule next report
    Simulator::Schedule(MilliSeconds(STATISTIC_INTERVAL_MILLISECONDS), &Cybertwin::CybertwinCommModelStatistical, this);
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
        Simulator::Schedule(MilliSeconds(STATISTIC_TIME_INTERVAL),
                            &Cybertwin::UpdateRxSizePerSecond,
                            this,
                            conn->m_peerCyberID);
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
        Simulator::Schedule(MilliSeconds(STATISTIC_TIME_INTERVAL),
                            &Cybertwin::UpdateRxSizePerSecond,
                            this,
                            conn->m_peerCyberID);
        Simulator::ScheduleNow(&Cybertwin::CybertwinServerBulkSend, this, conn);
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
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: connection error with " << peerAddr);
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
        STREAMID_t streamId = m_pendingConnectionsReverse[sock];
        NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: stream is successfully established");

        // erase the connection from pendingConnections, and insert it to txConnections
        m_pendingConnections.erase(streamId);
        m_pendingConnectionsReverse.erase(sock);
        m_txConnections[streamId] = sock;
        m_txConnectionsReverse[sock] = streamId;

        // after connection created, we schedule a send event to send pending packets
        Simulator::ScheduleNow(&Cybertwin::SendPendingPackets, this, streamId);
    }
    else
    {
        // case[2]: socket not found in pending connections means DataServer received a
        //          request and successfully created a connection
        Address peeraddr;
        sock->GetPeerName(peeraddr);

        NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId
                                  << "]: DataServer received a connection request from " << peeraddr
                                  << " and successfully created a connection");
        m_rxConnections.insert(sock);
        m_rxConnectionsReverse[sock] = peeraddr;
        m_rxSizePerSecond[sock] = 0;

        //Simulator::ScheduleNow(&Cybertwin::CybertwinServerBulkSend, this, sock);
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
                              << "]: received packet from " << m_rxConnectionsReverse[sock]
                              << " with size " << packet->GetSize() << " bytes");
        // TODO: what to do next? Send to client or save to buffer.
        m_rxSizePerSecond[sock] += packet->GetSize();


    }
}

void
Cybertwin::SpNormalCloseCallback(Ptr<Socket> sock)
{
    Address peeraddr;
    sock->GetPeerName(peeraddr);
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: connection closed with " << peeraddr);
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
        NS_LOG_ERROR("Cybertwin[" << m_cybertwinId << "]: connection closed with " << peeraddr
                                  << " but no such connection found");
    }
}

void
Cybertwin::SpErrorCloseCallback(Ptr<Socket> sock)
{
    Address peeraddr;
    sock->GetPeerName(peeraddr);
    NS_LOG_WARN("Cybertwin[" << m_cybertwinId << "]: connection error with " << peeraddr);
}

#endif

//***************************************************************************************
//*                     Handle incoming connections from cloud                          *
//***************************************************************************************
void
Cybertwin::GloballyListen()
{
#if MDTP_ENABLED
    // init data server and listen for incoming connections
    m_dtServer = new CybertwinDataTransferServer();
    m_dtServer->Setup(GetNode(), m_cybertwinId, m_globalInterfaces);
    m_dtServer->Listen();
    m_dtServer->SetNewConnectCreatedCallback(
        MakeCallback(&Cybertwin::NewMpConnectionCreatedCallback, this));
#else
    // init data server and listen for incoming connections
    m_dtServer = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
    // FIXME: attention, the port number is hard-coded here
    m_dtServer->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_globalInterfaces[0].second));
    m_dtServer->SetAcceptCallback(MakeCallback(&Cybertwin::NewSpConnectionRequestCallback, this),
                                  MakeCallback(&Cybertwin::NewSpConnectionCreatedCallback, this));
    m_dtServer->Listen();
#endif
}

//***************************************************************************************
//*                    Cybertwin Comm model Traffic Shaping                             *  
//***************************************************************************************

/**
 * @brief Cybertwin::CybertwinCommModelTraffic
 * @param speed in Mbps
 */
void
Cybertwin::CybertwinCommModelTrafficShaping(uint32_t speed)
{
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: traffic shaping with speed " << speed << " Mbps");
    // schedule a timer to generate token
    double interval_millisecond = (1 / ((speed * 1000000.0) / (500.0 * 8))) * 1000000.0; // 计算token生成间隔
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: token generate interval is " << interval_millisecond << " us");
    Simulator::ScheduleNow(&Cybertwin::GenerateToken, this, interval_millisecond);

    // schedule a timer to consume token
    Simulator::ScheduleNow(&Cybertwin::ConsumeToken, this);

    // schedule a statistic timer
    Simulator::Schedule(MilliSeconds(STATISTIC_INTERVAL_MILLISECONDS), &Cybertwin::TrafficShapingStatistical, this);
}

void
Cybertwin::GenerateToken(double interval)
{
    m_tokenBucket++;
    NS_LOG_LOGIC("Cybertwin[" << m_cybertwinId << "]: token bucket size is " << m_tokenBucket);
    m_tokenGeneratorEvent = Simulator::Schedule(MicroSeconds(interval), &Cybertwin::GenerateToken, this, interval);
}

void
Cybertwin::ConsumeToken()
{
    if (m_statisticalEnd && m_tsPktQueue.empty())
    {
        StopTrafficShaping();
        return ;
    }
    if (m_tokenBucket > 0 && !m_tsPktQueue.empty())
    {
        m_tokenBucket--;
        NS_LOG_LOGIC("Cybertwin[" << m_cybertwinId << "]: token bucket size is " << m_tokenBucket);
        Ptr<Packet> pkt = m_tsPktQueue.front();
        m_consumeBytes +=  pkt->GetSize();
        m_tsPktQueue.pop();
    }else
    {
        NS_LOG_LOGIC("Cybertwin[" << m_cybertwinId << "]: token bucket size is " << m_tokenBucket
                                   << ", pkt queue size is " << m_tsPktQueue.size());
    }

    // try to consume every 1us
    m_consumerEvent = Simulator::Schedule(MicroSeconds(10), &Cybertwin::ConsumeToken, this);
}

void
Cybertwin::TrafficShapingStatistical()
{
    Time now = Simulator::Now();
    Time interval = now - m_lastShapingTime;
    m_lastShapingTime = now;

    double thoughput = m_consumeBytes * 8 / interval.GetSeconds() / 1000000.0; // Mbps
    m_logStream << "Cybertwin[" << m_cybertwinId << "]: at " << (now - m_startTime).GetMilliSeconds()
                << " ms, traffic shaping thoughput is " << thoughput << " Mbps" << std::endl;
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: at " << (now - m_startTime).GetMilliSeconds()
                              << " ms, traffic shaping thoughput is " << thoughput << " Mbps");
    m_consumeBytes = 0;
    m_statisticalEvent = Simulator::Schedule(MilliSeconds(STATISTIC_INTERVAL_MILLISECONDS), &Cybertwin::TrafficShapingStatistical, this);
}

void
Cybertwin::StopTrafficShaping()
{
    Simulator::Cancel(m_tokenGeneratorEvent);
    Simulator::Cancel(m_consumerEvent);
    Simulator::Cancel(m_statisticalEvent);
}

//***************************************************************************************
//*                    Cybertwin Comm model Traffic Policing                            *
//***************************************************************************************

void
Cybertwin::CybertwinCommModelTrafficPolicing()
{
    NS_LOG_DEBUG("Cybertwin[" << m_cybertwinId << "]: traffic policing");
}





} // namespace ns3
