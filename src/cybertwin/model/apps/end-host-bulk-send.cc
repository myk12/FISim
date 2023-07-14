#include "ns3/end-host-bulk-send.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("EndHostBulkSend");

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
            .AddAttribute("CybertwinAddr",
                          "Cybertwin address.",
                          Ipv4AddressValue(),
                          MakeIpv4AddressAccessor(&EndHostBulkSend::m_cybertwinAddr),
                          MakeIpv4AddressChecker())
            .AddAttribute("CybertwinPort",
                          "Cybertwin port.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&EndHostBulkSend::m_cybertwinPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("TotalBytes",
                          "Total bytes to send.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&EndHostBulkSend::m_totalBytes),
                          MakeUintegerChecker<uint32_t>());
    return tid;
}

void
EndHostBulkSend::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Starting EndHostBulkSend.");

    // Connect to Cybertwin
    ConnectCybertwin();
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
EndHostBulkSend::ConnectCybertwin()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Connecting to Cybertwin.");

    // Check if Cybertwin address is set
    Ptr<CybertwinEndHost> host = DynamicCast<CybertwinEndHost>(GetNode());
    if (!host)
    {
        NS_LOG_ERROR("Node is not a Cybertwin end host.");
        return;
    }

    if (!host->GetCybertwinStatus())
    {
        // Cybertwin is not connected
        NS_LOG_ERROR("Cybertwin is not connected. Wait for 1 second.");
        Simulator::Schedule(Seconds(1.0), &EndHostBulkSend::ConnectCybertwin, this);
    }
    else
    {
        m_cybertwinAddr = host->GetUpperNodeAddress();
        m_cybertwinPort = host->GetCybertwinPort();
        // Cybertwin is connected, connect and send data
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
    NS_LOG_DEBUG("Connection succeeded.");

    m_socket->SetRecvCallback(MakeCallback(&EndHostBulkSend::RecvData, this));
    // Send data
    SendData();
}

void
EndHostBulkSend::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("Connection failed.");
}

void
EndHostBulkSend::ConnectionNormalClosed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("Connection closed normally.");
}

void
EndHostBulkSend::ConnectionErrorClosed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("Connection closed with error.");
}

void
EndHostBulkSend::RecvData(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("Received data.");

    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        NS_LOG_DEBUG("Received " << packet->GetSize() << " bytes from "
                                 << InetSocketAddress::ConvertFrom(from).GetIpv4());
    }
}

void
EndHostBulkSend::SendData()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("Sending data.");
    if (m_sentBytes >= m_totalBytes)
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

    NS_LOG_DEBUG("Sending Packet size: " << packet->GetSize());

    // Send packet
    m_sentBytes += m_socket->Send(packet);

    // Schedule next send
    Simulator::Schedule(Seconds(1.0), &EndHostBulkSend::SendData, this);
}

} // namespace ns3