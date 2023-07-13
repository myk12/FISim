#include "ns3/download-client.h"

namespace ns3
{
DownloadClient::DownloadClient(uint64_t cuid) : m_cuid(cuid)
{
  // TODO: Initialize m_nameResolutionService

  // Resolve CUID to server address
  ResolveCUID();
}

DownloadClient::~DownloadClient()
{
}

void DownloadClient::StartApplication()
{
    NS_LOG_FUNCTION(this);
    m_endHost = DynamicCast<CybertwinEndHost>(GetNode());
    if (!m_endHost)
    {
        NS_FATAL_ERROR("Node is not a CybertwinEndHost.");
        return ;
    }

    ConnectCybertwin();
}

void
DownloadClient::ConnectCybertwin()
{
    if (!m_endHost->CybertwinCreated())
    {
        NS_LOG_DEBUG("Cybertwin not created yet.");
        // schedule next second
        Simulator::Schedule(Seconds(1), &DownloadClient::ConnectCybertwin, this);
        return;
    }

    // Get Cybertwin Socket
    CYBERTWINID_t selfID = endHost->GetCybertwinId();
    uint16_t cybertwinPort = endHost->GetCybertwinPort();
    Ipv4Address cybertwinAddr = endHost->GetUpperNodeAddress();
    
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        m_socket->Bind();
        m_socket->Connect(InetSocketAddress(cybertwinAddr, cybertwinPort));
        m_socket->SetRecvCallback(MakeCallback(&DownloadClient::HandleRead, this));
    }

  
}

void DownloadClient::StopApplication()
{
  if (m_socket)
    m_socket->Close();
}

void DownloadClient::ResolveCUID()
{
  // TODO: Implement CUID resolution logic using m_nameResolutionService

  // Example: Hardcode server address and port for testing
  Ipv4Address serverIp("192.168.0.1");
  uint16_t serverPort = 5000;
  m_serverAddress = InetSocketAddress(serverIp, serverPort);
}

} // namespace ns3
