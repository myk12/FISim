#include "ns3/cybertwin-app-download-server.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinDownloadServer");
NS_OBJECT_ENSURE_REGISTERED(CybertwinDownloadServer);

TypeId
CybertwinDownloadServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinDownloadServer")
                            .SetParent<Application>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinDownloadServer>()
                            .AddAttribute("CybertwinID",
                                          "Cybertwin ID of the server.",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&CybertwinDownloadServer::m_cybertwinID),
                                          MakeUintegerChecker<uint64_t>())
                            .AddAttribute("ListenPort",
                                          "Port to listen on.",
                                          UintegerValue(80),
                                          MakeUintegerAccessor(&CybertwinDownloadServer::m_serverPort),
                                          MakeUintegerChecker<uint16_t>())
                            .AddAttribute("MaxBytes",
                                          "Maximum bytes to send.",
                                          UintegerValue(0),
                                          MakeUintegerAccessor(&CybertwinDownloadServer::m_maxBytes),
                                          MakeUintegerChecker<uint64_t>());
    return tid;
}

CybertwinDownloadServer::CybertwinDownloadServer()
{
    NS_LOG_DEBUG("[CybertwinDownloadServer] create CybertwinDownloadServer.");
}

CybertwinDownloadServer::CybertwinDownloadServer(CYBERTWINID_t cybertwinID, CYBERTWIN_INTERFACE_LIST_t interfaces)
{
    NS_LOG_DEBUG("[CybertwinDownloadServer] create CybertwinDownloadServer.");
    NS_LOG_DEBUG("[CybertwinDownloadServer] Cybertwin ID: " << cybertwinID);
    for (auto it = interfaces.begin(); it != interfaces.end(); ++it)
    {
        NS_LOG_DEBUG("[CybertwinDownloadServer] Interface: " << it->first << " " << it->second);
    }
}

CybertwinDownloadServer::~CybertwinDownloadServer()
{
    NS_LOG_DEBUG("[CybertwinDownloadServer] destroy CybertwinDownloadServer.");
}

void
CybertwinDownloadServer::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[CybertwinDownloadServer] Starting CybertwinDownloadServer.");

    // Init interface list: self ipv4 address and listening port
    for (auto globalIp : GetNode()->GetObject<CybertwinNode>()->GetGlobalIpList())
    {
        m_interfaces.push_back(std::make_pair(globalIp, m_serverPort));
    }

    // Get CNRS app and insert the cybertwin ID
    Ptr<CybertwinNode> node = GetNode()->GetObject<CybertwinNode>();
    Ptr<NameResolutionService> cnrs = node->GetCNRSApp();
    NS_ASSERT(cnrs);
    NS_LOG_DEBUG("[CybertwinDownloadServer] Inserting Cybertwin " << m_cybertwinID << " into CNRS.");
    cnrs->InsertCybertwinInterfaceName(m_cybertwinID, m_interfaces);

    // Init data server and listen for incoming connections
    m_serverSocket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
    m_serverSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_serverPort));
    m_serverSocket->SetAcceptCallback(MakeCallback(&CybertwinDownloadServer::ConnRequestCallback, this),
                                      MakeCallback(&CybertwinDownloadServer::ConnCreatedCallback, this));
    m_serverSocket->SetCloseCallbacks(MakeCallback(&CybertwinDownloadServer::NormalCloseCallback, this),
                                        MakeCallback(&CybertwinDownloadServer::ErrorCloseCallback, this));
    m_serverSocket->Listen();

    NS_LOG_DEBUG("[CybertwinDownloadServer] Server is listening on " << m_serverPort);
}

void
CybertwinDownloadServer::StopApplication()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("[CybertwinDownloadServer] Stopping CybertwinDownloadServer.");

    if (m_serverSocket)
    {
        m_serverSocket->Close();
    }
}

bool
CybertwinDownloadServer::ConnRequestCallback(Ptr<Socket> socket, const Address& from)
{
    NS_LOG_FUNCTION(this << socket << from);
    // accept all connections
    return true;
}

void
CybertwinDownloadServer::ConnCreatedCallback(Ptr<Socket> socket, const Address& from)
{
    NS_LOG_FUNCTION(this << socket << from);
    // put into the map
    NS_LOG_DEBUG("[CybertwinDownloadServer] New connection with "
                 << InetSocketAddress::ConvertFrom(from).GetIpv4() << ":"
                 << InetSocketAddress::ConvertFrom(from).GetPort());
    NS_LOG_DEBUG("[CybertwinDownloadServer] Send " << m_maxBytes << " bytes to "
                                               << InetSocketAddress::ConvertFrom(from).GetIpv4() << ":"
                                               << InetSocketAddress::ConvertFrom(from).GetPort());
    socket->SetRecvCallback(MakeCallback(&CybertwinDownloadServer::RecvCallback, this));
    
    // send data
    m_sendBytes[socket] = 0;
    Simulator::ScheduleNow(&CybertwinDownloadServer::SendData, this, socket);
}

void
CybertwinDownloadServer::RecvCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_INFO("[CybertwinDownloadServer] RecvCallback");
}

void
CybertwinDownloadServer::NormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Address peername;
    socket->GetPeerName(peername);
    NS_LOG_DEBUG("[CybertwinDownloadServer] Normal Connection with "
                 << InetSocketAddress::ConvertFrom(peername).GetIpv4() << ":"
                 << InetSocketAddress::ConvertFrom(peername).GetPort() << " closed.");
}

void
CybertwinDownloadServer::ErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("[CybertwinDownloadServer] Error Connection closed.");
}

void
CybertwinDownloadServer::SendData(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_INFO("[CybertwinDownloadServer] SendData");
    if (!socket)
    {
        NS_LOG_ERROR("[CybertwinDownloadServer] Connection closed.");
        return;
    }

    if (m_sendBytes[socket] >= m_maxBytes)
    {
        NS_LOG_DEBUG("[CybertwinDownloadServer] Send "
                     << m_sendBytes[socket] << " bytes");
        socket->Close();
        return;
    }

    // send data
    Ptr<Packet> packet = Create<Packet>(SYSTEM_PACKET_SIZE);
    int32_t sendSize = socket->Send(packet);
    if (sendSize <= 0)
    {
        NS_LOG_ERROR("[CybertwinDownloadServer] Send failed.");
        return;
    }

    m_sendBytes[socket] += sendSize;
    Simulator::Schedule(MilliSeconds(1), &CybertwinDownloadServer::SendData, this, socket);
}

} // namespace ns3
