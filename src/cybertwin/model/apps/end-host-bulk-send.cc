#include "ns3/end-host-bulk-send.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("EndHostBulkSend");
NS_OBJECT_ENSURE_REGISTERED(EndHostBulkSend);

EndHostBulkSend::EndHostBulkSend()
{
    m_trafficPattern = TRAFFIC_PATTERN_PARETO;
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
                          UintegerValue(1024*1024),
                          MakeUintegerAccessor(&EndHostBulkSend::m_totalBytes),
                          MakeUintegerChecker<uint32_t>());
    return tid;
}

void
EndHostBulkSend::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Starting EndHostBulkSend.");

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
    
    default:
        break;
    }

}

void
EndHostBulkSend::ConnectCybertwin()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Connecting to Cybertwin.");

    // Check if Cybertwin address is set
    Ptr<CybertwinEndHost> host = DynamicCast<CybertwinEndHost>(GetNode());
    if (!host)
    {
        NS_LOG_ERROR("[App][EndHostBulkSend] Node is not a CybertwinEndHost.");
        return;
    }

    if (host->isCybertwinCreated() == false)
    {
        // Cybertwin is not created
        NS_LOG_ERROR("[App][EndHostBulkSend] Cybertwin is not connected. Wait for 1 millisecond.");
        Simulator::Schedule(MilliSeconds(1.0), &EndHostBulkSend::ConnectCybertwin, this);
    }
    else
    {
        m_cybertwinAddr = host->GetUpperNodeAddress();
        m_cybertwinPort = host->GetCybertwinPort();
        NS_LOG_DEBUG("[App][EndHostBulkSend] Cybertwin is created. Try to connect to Cybertwin " << m_cybertwinAddr << ":" << m_cybertwinPort);
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

    m_socket->SetRecvCallback(MakeCallback(&EndHostBulkSend::RecvData, this));
    // Send data
    SendData();
}

void
EndHostBulkSend::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Connection failed with error " << socket->GetErrno());
}

void
EndHostBulkSend::ConnectionNormalClosed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Connection closed normally.");
}

void
EndHostBulkSend::ConnectionErrorClosed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Connection closed with error.");
}

void
EndHostBulkSend::RecvData(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Received data.");

    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        NS_LOG_DEBUG("[App][EndHostBulkSend] Received packet from " << InetSocketAddress::ConvertFrom(from).GetIpv4());
    }
}

void
EndHostBulkSend::SendData()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[App][EndHostBulkSend] Sending data.");
    if (m_totalBytes != 0 && m_sentBytes >= m_totalBytes)
    {
        NS_LOG_DEBUG("Sent " << m_sentBytes << " bytes.");
        m_socket->Close();
        return;
    }

    // Create packet
    Ptr<Packet> packet = Create<Packet>();

    CybertwinHeader header;
    header.SetCybertwin(1000);
    header.SetPeer(2000);
    header.SetCommand(CYBERTWIN_HEADER_DATA);

    packet->AddHeader(header);
    packet->AddPaddingAtEnd(SYSTEM_PACKET_SIZE - header.GetSerializedSize());

    Time now = Simulator::Now();
    NS_LOG_DEBUG(now << "Sending Packet size: " << packet->GetSize());

    // Send packet
    m_sentBytes += m_socket->Send(packet);

    // Schedule next send
    double interArrivalTime = m_randomVariableStream->GetValue();
    Simulator::Schedule(MicroSeconds(interArrivalTime), &EndHostBulkSend::SendData, this);
}

} // namespace ns3