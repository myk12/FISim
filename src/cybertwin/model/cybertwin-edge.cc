#include "cybertwin-edge.h"

#include "ns3/callback.h"
#include "ns3/ipv4-header.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/uinteger.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinEdge");
NS_OBJECT_ENSURE_REGISTERED(CybertwinController);
NS_OBJECT_ENSURE_REGISTERED(CybertwinTrafficManager);

TypeId
CybertwinController::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinController")
                            .SetParent<Application>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinController>()
                            .AddAttribute("LocalAddress",
                                          "The address on which to bind the listening socket",
                                          AddressValue(),
                                          MakeAddressAccessor(&CybertwinController::m_localAddr),
                                          MakeAddressChecker())
                            .AddAttribute("LocalPort",
                                          "The port on which the application sends data",
                                          UintegerValue(443),
                                          MakeUintegerAccessor(&CybertwinController::m_localPort),
                                          MakeUintegerChecker<uint16_t>());
    return tid;
}

CybertwinController::CybertwinController()
    : m_socket(nullptr)
{
}

CybertwinController::~CybertwinController()
{
}

void
CybertwinController::DoDispose()
{
    NS_LOG_FUNCTION(GetNode()->GetId());
    if (!Simulator::IsFinished())
    {
        // Close sockets
        StopApplication();
    }
    m_socket = nullptr;
    m_cybertwinTable.clear();
    Application::DoDispose();
}

void
CybertwinController::StartApplication()
{
    NS_LOG_FUNCTION(GetNode()->GetId());
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        DoSocketMethod(&Socket::Bind, m_socket, m_localAddr, m_localPort);
    }
    m_socket->SetAcceptCallback(MakeCallback(&CybertwinController::HostConnecting, this),
                                MakeCallback(&CybertwinController::HostConnected, this));
    m_socket->SetCloseCallbacks(MakeCallback(&CybertwinController::NormalHostClose, this),
                                MakeCallback(&CybertwinController::ErrorHostClose, this));
    m_socket->Listen();
}

void
CybertwinController::StopApplication()
{
    NS_LOG_FUNCTION(GetNode()->GetId());
    if (m_socket)
    {
        m_socket->Close();
        m_socket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                    MakeNullCallback<void, Ptr<Socket>, const Address&>());
        m_socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                    MakeNullCallback<void, Ptr<Socket>>());
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_socket->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
    }
}

bool
CybertwinController::HostConnecting(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket << address);
    return true;
}

void
CybertwinController::HostConnected(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket << address);
    socket->SetRecvCallback(MakeCallback(&CybertwinController::ReceiveFromHost, this));
    socket->SetCloseCallbacks(MakeCallback(&CybertwinController::NormalHostClose, this),
                              MakeCallback(&CybertwinController::ErrorHostClose, this));
    ReceiveFromHost(socket);
}

void
CybertwinController::ReceiveFromHost(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket);
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            break;
        }

        CybertwinHeader header;
        packet->PeekHeader(header);
        NS_ASSERT_MSG(!header.isDataPacket(),
                      "--[Ctrl-" << GetNode()->GetId() << "]: received invalid header type");

        switch (header.GetCommand())
        {
        case HOST_CONNECT:
            BornCybertwin(header, socket);
            NS_LOG_DEBUG("--[Ctrl-" << GetNode()->GetId() << "]: cybertwin created");
            break;
        case HOST_DISCONNECT:
            KillCybertwin(header, socket);
            NS_LOG_DEBUG("--[Ctrl-" << GetNode()->GetId() << "]: cybertwin removed");
            break;
        default:
            NS_LOG_ERROR("--[Ctrl-" << GetNode()->GetId() << "]: unknown Command");
            break;
        }
    }
}

void
CybertwinController::BornCybertwin(const CybertwinHeader& header, Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << header);
    CYBERTWINID_t cuid = header.GetCybertwin();
    if (m_cybertwinTable.find(cuid) == m_cybertwinTable.end())
    {
        Ptr<Cybertwin> cybertwin = Create<Cybertwin>(cuid, socket, m_localAddr);
        m_cybertwinTable[cuid] = cybertwin;
        GetNode()->AddApplication(cybertwin);
    }
    else
    {
        // TODO: supports multiple connections to a single cybertwin
    }
}

void
CybertwinController::KillCybertwin(const CybertwinHeader& reqHeader, Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << reqHeader);
    CYBERTWINID_t cuid = reqHeader.GetCybertwin();
    if (m_cybertwinTable.find(cuid) != m_cybertwinTable.end())
    {
        Ptr<Cybertwin> cybertwin = m_cybertwinTable[cuid];
        cybertwin->SetStopTime(Seconds(0));
        m_cybertwinTable.erase(cuid);
    }
    else
    {
        // TODO: supports multiple connections to a single cybertwin
        NS_LOG_ERROR("--[Ctrl-" << GetNode()->GetId() << "]: cybertwin not found");
    }
}

void
CybertwinController::NormalHostClose(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket);
    if (socket != m_socket)
    {
        socket->ShutdownSend();
    }
}

void
CybertwinController::ErrorHostClose(Ptr<Socket> socket)
{
    NS_LOG_ERROR("--[Ctrl-" << GetNode()->GetId()
                            << "]: a socket error occurs:" << socket->GetErrno());
}

CybertwinTrafficManager::CybertwinTrafficManager()
{
}

CybertwinTrafficManager::~CybertwinTrafficManager()
{
}

TypeId
CybertwinTrafficManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinTrafficManager")
                            .SetParent<Application>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinTrafficManager>();
    return tid;
}

void
CybertwinTrafficManager::DoDispose()
{
    NS_LOG_FUNCTION(GetNode()->GetId());
    if (!Simulator::IsFinished())
    {
        StopApplication();
    }
    m_firewallTable.clear();
    Application::DoDispose();
}

void
CybertwinTrafficManager::StartApplication()
{
    NS_LOG_FUNCTION(this);
    GetNode()->SetCybertwinFirewall(MakeCallback(&CybertwinTrafficManager::InspectPacket, this));
}

void
CybertwinTrafficManager::StopApplication()
{
}

bool
CybertwinTrafficManager::InspectPacket(Ptr<NetDevice> device,
                                       Ptr<const Packet> packet,
                                       uint16_t protocol)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << device->GetIfIndex() << packet->ToString());
    CybertwinCreditTag creditTag;
    if (packet->PeekPacketTag(creditTag))
    {
        // packet comes from another cybertwin
        CYBERTWINID_t cuid = creditTag.GetPeer();
        CYBERTWINID_t src = creditTag.GetCybertwin();
        return m_firewallTable.find(cuid) != m_firewallTable.end() &&
               m_firewallTable[cuid].Filter(src, creditTag);
    }
    CybertwinCertTag certTag;
    if (packet->PeekPacketTag(certTag))
    {
        // packet comes from another cybertwin
        CYBERTWINID_t cuid = certTag.GetCybertwin();
        if (m_firewallTable.find(cuid) == m_firewallTable.end())
        {
            m_firewallTable[cuid] = CybertwinFirewall(cuid);
        }
        return m_firewallTable[cuid].Authenticate(certTag);
    }
    CybertwinTag idTag;
    if (packet->PeekPacketTag(idTag))
    {
        CYBERTWINID_t cuid = idTag.GetCybertwin();
        return m_firewallTable.find(cuid) != m_firewallTable.end() &&
               m_firewallTable[cuid].Forward(packet);
    }
    // non cybertwin packet
    return true;
}

CybertwinFirewall::CybertwinFirewall(CYBERTWINID_t cuid)
    : m_ingressCredit(0),
      m_cuid(cuid),
      m_isUsrAuthRequired(false),
      m_usrCuid(0),
      m_state(NOT_STARTED)
{
}

bool
CybertwinFirewall::Filter(CYBERTWINID_t src, const CybertwinCreditTag& creditTag)
{
    NS_LOG_FUNCTION(m_cuid << src << creditTag.ToString() << m_state << m_ingressCredit);
    if (m_state == PERMITTED && creditTag.GetCredit() >= m_ingressCredit)
    {
        // TODO: record src and check if it is malicious
        return true;
    }
    NS_LOG_DEBUG("Global packet from " << src << " got blocked");
    return false;
}

bool
CybertwinFirewall::Forward(Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(m_cuid << m_state << m_ingressCredit);
    if (m_state == PERMITTED)
    {
        // TODO: examine packet content
        return true;
    }
    NS_LOG_DEBUG("Local packet from" << m_cuid << " got blocked");
    return false;
}

bool
CybertwinFirewall::Authenticate(const CybertwinCertTag& cert)
{
    if (cert.GetIsValid())
    {
        m_ingressCredit = std::max(m_ingressCredit, cert.GetIngressCredit());
        m_isUsrAuthRequired = cert.GetIsUserRequired();
        if (m_isUsrAuthRequired)
        {
            m_usrCuid = cert.GetUser();
        }
        m_state = PERMITTED;
        // TODO: update the credit score of this cybertwin
        return true;
    }
    return false;
}

void
CybertwinFirewall::Dispose()
{
    m_state = NOT_STARTED;
}

} // namespace ns3