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

    uint32_t streamID = 0;
    for (auto cuid : m_cuidList)
    {
        DownloadStream stream;
        stream.SetNode(GetNode());
        stream.SetStreamID(streamID++);
        stream.SetTargetID(cuid);
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
    header.SetCommand(CYBERTWIN_HEADER_DATA);
    header.SetCybertwin(m_cuid);
    NS_LOG_DEBUG("[DownloadStream] 2Sending request to " << m_targetID << ".");
    header.SetPeer(1000);

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);
    packet->AddPaddingAtEnd(SYSTEM_PACKET_SIZE - header.GetSerializedSize());

    socket->Send(packet);
    header.PrintH(std::cout);
}

void
DownloadStream::RecvCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    while ((packet = socket->Recv()))
    {
        NS_LOG_DEBUG("[DownloadStream] Received packet from " << packet->GetSize() << " bytes.");
    }
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
}

void
DownloadStream::ConnectionErrorClosed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("[DownloadStream] Connection error closed.");
}



} // namespace ns3
