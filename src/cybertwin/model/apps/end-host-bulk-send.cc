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
                          UintegerValue(1024),
                          MakeUintegerAccessor(&EndHostBulkSend::m_totalBytes),
                          MakeUintegerChecker<uint32_t>());
    return tid;
}

void
EndHostBulkSend::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Starting EndHostBulkSend.");
    m_trafficPattern = TRAFFIC_PATTERN_EXPONENTIAL;

    Ptr<CybertwinNode> node = DynamicCast<CybertwinNode>(GetNode());
    if (!node)
    {
        NS_LOG_ERROR("[EndHostBulkSend] Node is not a CybertwinNode.");
        return;
    }

    m_trafficPattern = TRAFFIC_PATTERN_PARETO;
    OpenLogFile(node->GetLogDir(), "end-host-bulk-send.log");

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
    NS_LOG_DEBUG("Initializing random variable stream.");

    switch (m_trafficPattern)
    {
    case TRAFFIC_PATTERN_PARETO:
        NS_LOG_DEBUG("Pareto distribution.");
        m_randomVariableStream = CreateObject<ParetoRandomVariable>();
        m_randomVariableStream->SetAttribute("Shape", DoubleValue(2));
        m_randomVariableStream->SetAttribute("Scale", DoubleValue(5));
        break;
    case TRAFFIC_PATTERN_EXPONENTIAL:
        NS_LOG_DEBUG("Exponential distribution.");
        m_randomVariableStream = CreateObject<ExponentialRandomVariable>();
        m_randomVariableStream->SetAttribute("Mean", DoubleValue(10));
        m_randomVariableStream->SetAttribute("Bound", DoubleValue(0.0));
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
    m_startTime = Simulator::Now();
    NS_LOG_DEBUG("[App][EndHostBulkSend] Start sending data at " << m_startTime.GetSeconds() << " seconds.");
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
    if (m_totalBytes != 0 && m_sentBytes >= m_totalBytes)
    {
        NS_LOG_DEBUG("[App][EndHostBulkSend] Sent " << m_sentBytes << " bytes. Stop sending data at " << Simulator::Now().GetSeconds() << " seconds.");
        m_socket->Close();
        return;
    }
    
    if((Simulator::Now() - m_startTime).GetSeconds() >= END_HOST_BULK_SEND_TEST_TIME)
    {
        NS_LOG_DEBUG("[App][EndHostBulkSend] Sent " << m_sentBytes << " bytes. Stop sending data at " << Simulator::Now().GetSeconds() << " seconds.");
        m_socket->Close();
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

    NS_LOG_DEBUG(Simulator::Now() << "Sending Packet size: " << packet->GetSize());
    m_logStream << Simulator::Now() << " Sending Packet size: " << packet->GetSize() << std::endl;

    // Send packet
    int32_t size = m_socket->Send(packet);
    if (size < 0)
    {
        NS_LOG_INFO("[App][EndHostBulkSend] Error while sending packet << " << size << " error msg: " << m_socket->GetErrno() << ".");
    }else
    {
        m_sentBytes += size;
        NS_LOG_INFO("[App][EndHostBulkSend] Sent " << size << " bytes.");
    }

    // Schedule next send
    double interArrivalTime = m_randomVariableStream->GetValue();
    //NS_LOG_DEBUG("[App][EndHostBulkSend] Inter arrival time: " << interArrivalTime);
    Simulator::Schedule(MicroSeconds(interArrivalTime*10), &EndHostBulkSend::SendData, this);
}

} // namespace ns3