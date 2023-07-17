#include "ns3/end-host-bulk-send.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("EndHostBulkSend");
NS_OBJECT_ENSURE_REGISTERED(EndHostBulkSend);

EndHostBulkSend::EndHostBulkSend()
{
    NS_LOG_DEBUG("[EndHostBulkSend] create EndHostBulkSend.");
}

EndHostBulkSend::~EndHostBulkSend()
{
    NS_LOG_DEBUG("[EndHostBulkSend] destroy EndHostBulkSend.");
}

TypeId
EndHostBulkSend::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::EndHostBulkSend")
            .SetParent<Application>()
            .SetGroupName("Cybertwin")
            .AddConstructor<EndHostBulkSend>()
            .AddAttribute("TotalBytes",
                          "Total bytes to send.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&EndHostBulkSend::m_maxBytes),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("AverateSendRate",
                          "Average send rate in Mbps.",
                          DoubleValue(100),
                          MakeDoubleAccessor(&EndHostBulkSend::m_averageSendRate),
                          MakeDoubleChecker<double>());
    return tid;
}

void
EndHostBulkSend::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Starting EndHostBulkSend at time " << Simulator::Now().GetSeconds() << " seconds.");

    Ptr<CybertwinNode> node = DynamicCast<CybertwinNode>(GetNode());
    if (!node)
    {
        NS_LOG_ERROR("[EndHostBulkSend] Node is not a CybertwinNode.");
        return;
    }

    OpenLogFile(node->GetLogDir(), "end-host-bulk-send.log");
    m_trafficPattern = TRAFFIC_PATTERN_EXPONENTIAL;
    m_totalSendBytes = 0;
    m_sentBytes = 0;

    // Connect to Cybertwin
    ConnectCybertwin();

    // init random variable stream
    InitRandomVariableStream();
}

void
EndHostBulkSend::StopApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Stopping EndHostBulkSend.");

    CloseLogFile();

    if (m_socket)
    {
        m_socket->Close();
    }
}

void
EndHostBulkSend::InitRandomVariableStream()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[App][EndHostBulkSend] InitRandomVariableStream.");

    switch (m_trafficPattern)
    {
    case TRAFFIC_PATTERN_PARETO:
        NS_LOG_DEBUG("[App][EndHostBulkSend] Pareto distribution.");
        m_randomVariableStream = CreateObject<ParetoRandomVariable>();
        m_randomVariableStream->SetAttribute("Shape", DoubleValue(2));
        m_randomVariableStream->SetAttribute("Scale", DoubleValue(5));
        break;
    case TRAFFIC_PATTERN_EXPONENTIAL:
        NS_LOG_DEBUG("[App][EndHostBulkSend] Exponential distribution.");
        m_randomVariableStream = CreateObject<ExponentialRandomVariable>();
        m_randomVariableStream->SetAttribute("Mean", DoubleValue(40)); // 40 microseconds per packet
        break;
    
    default:
        break;
    }

}

void
EndHostBulkSend::ConnectCybertwin()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Connecting to Cybertwin.");
    m_logStream << Simulator::Now() << " Connecting to Cybertwin." << std::endl;

    // Check if Cybertwin address is set
    Ptr<CybertwinEndHost> host = DynamicCast<CybertwinEndHost>(GetNode());
    if (!host)
    {
        NS_LOG_ERROR("[App][EndHostBulkSend] Node is not a CybertwinEndHost.");
        m_logStream << Simulator::Now() << " Node is not a CybertwinEndHost." << std::endl;
        return;
    }

    if (host->isCybertwinCreated() == false)
    {
        // Cybertwin is not created
        NS_LOG_ERROR("[App][EndHostBulkSend] Cybertwin is not connected. Wait for 100 millisecond.");
        m_logStream << Simulator::Now() << " Cybertwin is not connected. Wait for 100 millisecond." << std::endl;
        Simulator::Schedule(MilliSeconds(100.0), &EndHostBulkSend::ConnectCybertwin, this);
    }
    else
    {
        m_cybertwinAddr = host->GetUpperNodeAddress();
        m_cybertwinPort = host->GetCybertwinPort();
        NS_LOG_DEBUG("[App][EndHostBulkSend] Cybertwin is created. Try to connect to Cybertwin " << m_cybertwinAddr << ":" << m_cybertwinPort);
        m_logStream << Simulator::Now() << " Cybertwin is created. Try to connect to Cybertwin " << m_cybertwinAddr << ":" << m_cybertwinPort << std::endl;
        // Cybertwin is created, connect and send data
        if (!m_socket)
        {
            m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
            m_socket->Bind();

            m_socket->SetConnectCallback(MakeCallback(&EndHostBulkSend::ConnectionSucceeded, this),
                                         MakeCallback(&EndHostBulkSend::ConnectionFailed, this));
            m_socket->SetCloseCallbacks(
                MakeCallback(&EndHostBulkSend::ConnectionNormalClosed, this),
                MakeCallback(&EndHostBulkSend::ConnectionErrorClosed, this));
            m_socket->Connect(InetSocketAddress(m_cybertwinAddr, m_cybertwinPort));
        }
    }
}

void
EndHostBulkSend::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Connection succeeded.");
    m_logStream << Simulator::Now() << " Connection succeeded." << std::endl;

    socket->SetRecvCallback(MakeCallback(&EndHostBulkSend::RecvData, this));
    // Send data
    Simulator::Schedule(MilliSeconds(10.0), &EndHostBulkSend::ThroughputLogger, this);
    m_startTime = Simulator::Now();
    SendData();
}

void
EndHostBulkSend::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Connection failed with error " << socket->GetErrno());
    m_logStream << Simulator::Now() << " Connection failed with error " << socket->GetErrno() << std::endl;
}

void
EndHostBulkSend::ConnectionNormalClosed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Connection closed normally.");
    m_logStream << Simulator::Now() << " Connection closed normally." << std::endl;
}

void
EndHostBulkSend::ConnectionErrorClosed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Connection closed with error.");
    m_logStream << Simulator::Now() << " Connection closed with error." << std::endl;
}

void
EndHostBulkSend::RecvData(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Received data.");
    m_logStream << Simulator::Now() << " Received data." << std::endl;

    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        NS_LOG_DEBUG("[App][EndHostBulkSend] Received packet from " << InetSocketAddress::ConvertFrom(from).GetIpv4());
        m_logStream << Simulator::Now() << " Received packet from " << InetSocketAddress::ConvertFrom(from).GetIpv4() << std::endl;
    }
}

void
EndHostBulkSend::SendData()
{
    NS_LOG_FUNCTION(this);
    //NS_LOG_DEBUG("[App][EndHostBulkSend] Sending data.");
    if (m_maxBytes != 0 && m_totalSendBytes >= m_maxBytes)
    {
        NS_LOG_DEBUG("[App][EndHostBulkSend] Sent " << m_totalSendBytes << " bytes. Stop sending data at " << Simulator::Now().GetSeconds() << " seconds.");
        EndSend();
        return;
    }
    
    if((Simulator::Now() - m_startTime).GetSeconds() >= END_HOST_BULK_SEND_TEST_TIME)
    {
        NS_LOG_DEBUG("[App][EndHostBulkSend] Sent " << m_totalSendBytes << " bytes. Stop sending data at " << Simulator::Now().GetSeconds() << " seconds.");
        EndSend();
        return;
    }

    // Send data
    // Create packet
    Ptr<Packet> packet = Create<Packet>();

    CybertwinHeader header;
    header.SetCybertwin(COMM_TEST_CYBERTWIN_ID);
    header.SetPeer(COMM_TEST_CYBERTWIN_ID);
    header.SetCommand(CYBERTWIN_HEADER_DATA);

    packet->AddHeader(header);
    packet->AddPaddingAtEnd(SYSTEM_PACKET_SIZE - header.GetSerializedSize());

    //m_logStream << Simulator::Now() << " Sending Packet size: " << packet->GetSize() << std::endl;
    // Schedule next send
    int32_t size = m_socket->Send(packet);

    // if send error, try to send again for 10 times
    if (size <= 0)
    {
        NS_LOG_INFO("[App][EndHostBulkSend] Sent error with code " << size <<" errorno: " << m_socket->GetErrno());

        for (int32_t i=0; i<10; i++)
        {
            // try to send again
            size = m_socket->Send(packet);
            if (size > 0)
            {
                m_logStream << Simulator::Now() << " Try to send again. Sent " << size << " bytes." << std::endl << "after " << i << " times." << std::endl;
                break;
            }
        }
    }

    if (size > 0)
    {
        //NS_LOG_DEBUG("[App][EndHostBulkSend] Sent " << size << " bytes.");
        m_logStream << Simulator::Now() << " Sent " << size << " bytes." << std::endl;
        m_totalSendBytes += size;
        m_sentBytes += size;
    }else
    {
        m_logStream << Simulator::Now() << " Sent error with code " << size << std::endl;
    }

    // schedule next send
    double interArrivalTime = m_randomVariableStream->GetValue();
    NS_LOG_INFO("[App][EndHostBulkSend] Inter arrival time: " << interArrivalTime);
    m_logStream << Simulator::Now() << " Inter arrival time: " << interArrivalTime << std::endl;
    Simulator::Schedule(MicroSeconds(interArrivalTime), &EndHostBulkSend::SendData, this);
}

void
EndHostBulkSend::EndSend()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[App][EndHostBulkSend] EndSend.");
    if (m_socket)
    {
        m_socket->Close();
    }

    if (m_loggerEvent.IsRunning())
    {
        Simulator::Cancel(m_loggerEvent);
    }

    m_logStream << Simulator::Now() << " EndSend." << std::endl;
}

void
EndHostBulkSend::ThroughputLogger()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[App][EndHostBulkSend] ThroughputLogger.");
    //m_logStream << Simulator::Now() << " ThroughputLogger." << std::endl;
    if (m_sentBytes == 0)
    {
        // No data sent, stop logging
        return;
    }

    Time now = Simulator::Now();
    double throughput = (m_sentBytes * 8.0) / (now - m_lastTime).GetSeconds() / 1000000.0;
    m_lastTime = now;
    m_sentBytes = 0;

    m_logStream << Simulator::Now() << "BulkSend Throughput: " << throughput << " Mbps." << std::endl;
    m_loggerEvent = Simulator::Schedule(MilliSeconds(10.0), &EndHostBulkSend::ThroughputLogger, this);
}

} // namespace ns3