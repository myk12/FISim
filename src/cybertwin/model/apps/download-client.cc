#include "ns3/download-client.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("DownloadClient");
NS_OBJECT_ENSURE_REGISTERED(DownloadClient);

//******************************************************************************
//*                         download client                                    *
//******************************************************************************

TypeId
DownloadClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DownloadClient")
            .SetParent<Application>()
            .SetGroupName("Cybertwin")
            .AddConstructor<DownloadClient>();
    return tid;
}

DownloadClient::DownloadClient()
{
    NS_LOG_DEBUG("[DownloadClient] create DownloadClient.");
}

DownloadClient::DownloadClient(std::vector<CYBERTWINID_t> cuids)
{
    NS_LOG_DEBUG("[DownloadClient] create DownloadClient.");
    m_cuidList = cuids;
}

DownloadClient::~DownloadClient()
{
}

void
DownloadClient::AddCUID(CYBERTWINID_t cuid)
{
    m_cuidList.push_back(cuid);
}

void
DownloadClient::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[DownloadClient] Starting DownloadClient.");

    StartDownloadStreams();
}

void
DownloadClient::StartDownloadStreams()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[DownloadClient] Starting DownloadClient.");
    m_endHost = DynamicCast<CybertwinEndHost>(GetNode());
    if (!m_endHost)
    {
        NS_FATAL_ERROR("Node is not a CybertwinEndHost.");
        return;
    }

    if (!m_endHost->isCybertwinCreated())
    {
        NS_LOG_DEBUG("[DownloadClient] Cybertwin not created yet.");
        Simulator::Schedule(Seconds(1.0), &DownloadClient::StartDownloadStreams, this);
        return;
    }

    Ipv4Address cybertwinAddress = m_endHost->GetUpperNodeAddress();
    uint16_t cybertwinPort = m_endHost->GetCybertwinPort();
    NS_LOG_DEBUG("[DownloadClient] Cybertwin created, address: " << cybertwinAddress << ":" << cybertwinPort << " Start download streams.");

    uint32_t streamID = 0;
    for (auto cuid : m_cuidList)
    {
        NS_LOG_DEBUG("[DownloadClient] Create download stream to " << cuid << ".");
        DownloadStream stream;
        stream.SetNode(GetNode());
        stream.SetStreamID(streamID++);
        stream.SetTargetID(cuid);
        stream.SetCUID(0);
        stream.SetCybertwin(cybertwinAddress, cybertwinPort);
        stream.Activate();
    }
}

void
DownloadClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
}

//******************************************************************************
//*                         download stream                                    *
//******************************************************************************

DownloadStream::DownloadStream()
{
    m_socket = nullptr;
}

DownloadStream::~DownloadStream()
{
}

void
DownloadStream::SetNode(Ptr<Node> node)
{
    m_node = node;
}

void
DownloadStream::SetStreamID(uint32_t streamID)
{
    m_streamID = streamID;
}

void
DownloadStream::SetTargetID(CYBERTWINID_t targetID)
{
    m_targetID = targetID;
}

void
DownloadStream::SetCybertwin(Ipv4Address cybertwinAddress, uint16_t cybertwinPort)
{
    m_cybertwinAddress = cybertwinAddress;
    m_cybertwinPort = cybertwinPort;
}

void
DownloadStream::SetCUID(CYBERTWINID_t cuid)
{
    m_cuid = cuid;
}

void
DownloadStream::Activate()
{
    NS_LOG_FUNCTION(this);  
    NS_LOG_DEBUG("[DownloadStream] Activate stream " << m_streamID << " to " << m_targetID);

    m_socket = Socket::CreateSocket(m_node, TcpSocketFactory::GetTypeId());
    m_socket->Bind();
    m_socket->Connect(InetSocketAddress(m_cybertwinAddress, m_cybertwinPort));
    NS_LOG_DEBUG("[DownloadStream] Connecting to " << m_cybertwinAddress << ":" << m_cybertwinPort);
    m_socket->SetConnectCallback(MakeCallback(&DownloadStream::ConnectionSucceeded, this),
                                 MakeCallback(&DownloadStream::ConnectionFailed, this));
    m_socket->SetCloseCallbacks(MakeCallback(&DownloadStream::ConnectionNormalClosed, this),
                                MakeCallback(&DownloadStream::ConnectionErrorClosed, this));
    NS_LOG_DEBUG("[DownloadStream] " << this);
    NS_LOG_DEBUG("[DownloadStream] Connecting to " << m_targetID);
}

void
DownloadStream::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[DownloadStream] " << this);
    NS_LOG_DEBUG("[DownloadStream] Connection succeeded.");
    NS_LOG_DEBUG("[DownloadStream] 1Sending request to " << m_targetID << ".");

    // set recv callback
    socket->SetRecvCallback(MakeCallback(&DownloadStream::RecvCallback, this));

    // send request
    CybertwinHeader header;
    header.SetCommand(CREATE_STREAM);
    header.SetSelfID(m_cuid);
    NS_LOG_DEBUG("[DownloadStream] 2Sending request to " << m_targetID << ".");
    header.SetPeerID(1000);
    header.Print(std::cout);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    packet->AddPaddingAtEnd(SYSTEM_PACKET_SIZE - header.GetSerializedSize());
    int32_t ret = socket->Send(packet);
    if (ret <= 0)
    {
        NS_LOG_ERROR("[DownloadStream] Send request failed.");
        return;
    }

    // start statistical
    m_lastTime = Simulator::Now();
    m_intervalBytes = 0;
    //m_statisticalEvent = Simulator::Schedule(MilliSeconds(10), &DownloadStream::DownloadThroughputStatistical, this);
}

void
DownloadStream::RecvCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    while ((packet = socket->Recv()))
    {
        NS_LOG_DEBUG("[DownloadStream] Received packet from " << m_targetID << ". Size: " << packet->GetSize() << " bytes.");
        //m_intervalBytes += packet->GetSize();
    }

    NS_LOG_DEBUG("[DownloadStream] Received packet size: " << m_intervalBytes << " bytes.");
}

void
DownloadStream::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("[DownloadStream] Connection failed.");
}

void
DownloadStream::ConnectionNormalClosed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[DownloadStream] Connection closed.");
    if (m_statisticalEvent.IsRunning())
    {
        Simulator::Cancel(m_statisticalEvent);
    }
}

void
DownloadStream::ConnectionErrorClosed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("[DownloadStream] Connection error closed.");
}

void
DownloadStream::DownloadThroughputStatistical()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[DownloadStream] Download throughput statistical.");

    Time now = Simulator::Now();
    Time interval = now - m_lastTime;

    double throughput = (double)m_intervalBytes * 8.0 / interval.GetSeconds() / 1024.0 / 1024.0;

    NS_LOG_INFO("[DownloadStream] Download throughput: " << throughput << " Mbps.");

    m_intervalBytes = 0;
    m_lastTime = now;

    m_statisticalEvent = Simulator::Schedule(MilliSeconds(10), &DownloadStream::DownloadThroughputStatistical, this);
}

} // namespace ns3
