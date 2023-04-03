#include "cybertwin-edge.h"

#include "cybertwin-cert.h"

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
        packet->RemoveHeader(header);
        NS_ASSERT_MSG(!header.isDataPacket(),
                      "--[Ctrl-" << GetNode()->GetId() << "]: received invalid header type");

        switch (header.GetCommand())
        {
        case HOST_CONNECT:
            BornCybertwin(socket, packet);
            NS_LOG_DEBUG("--[Ctrl-" << GetNode()->GetId() << "]: cybertwin created");
            break;
        case HOST_DISCONNECT:
            KillCybertwin(socket, header);
            NS_LOG_DEBUG("--[Ctrl-" << GetNode()->GetId() << "]: cybertwin removed");
            break;
        default:
            NS_LOG_ERROR("--[Ctrl-" << GetNode()->GetId() << "]: unknown Command");
            break;
        }
    }
}

void
CybertwinController::BornCybertwin(Ptr<Socket> socket, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket);

    CybertwinCert devCert, usrCert;
    CYBERTWINID_t cuid;
    try
    {
        packet->RemoveHeader(devCert);
        if (devCert.GetIsCertValid() == false)
        {
            throw std::runtime_error("Invalid device certificate");
        }

        cuid = devCert.GetCybertwinId();
        if (m_cybertwinTable.find(cuid) != m_cybertwinTable.end())
        {
            // TODO: Should return a rspHeader with information related to the existed cybertwin
            throw std::runtime_error("");
        }

        if (devCert.GetIsUserAuthRequired())
        {
            // TODO: Check if there's more certificate
            packet->RemoveHeader(usrCert);
            if (!usrCert.GetIsCertValid())
            {
                throw std::runtime_error("Invalid user certificate");
            }
        }

        // create a cybertwin
        Ptr<Cybertwin> cybertwin = Create<Cybertwin>(cuid, socket);
        m_cybertwinTable[cuid] = cybertwin;

        // by default, the application will start running right away
        GetNode()->AddApplication(cybertwin);
    }
    catch (const std::exception& e)
    {
        CybertwinHeader rspHeader;
        rspHeader.SetCommand(CYBERTWIN_CONNECT_ERROR);
        // create response packet
        Ptr<Packet> rspPacket = Create<Packet>(0);
        rspPacket->AddHeader(rspHeader);
        socket->Send(rspPacket);
    }
}

void
CybertwinController::KillCybertwin(Ptr<Socket> socket, const CybertwinHeader& reqHeader)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket << reqHeader);
    CYBERTWINID_t cuid = reqHeader.GetCybertwin();
    if (m_cybertwinTable.find(cuid) != m_cybertwinTable.end())
    {
        Ptr<Cybertwin> cybertwin = m_cybertwinTable[cuid];
        cybertwin->SetStopTime(Seconds(0));
        m_cybertwinTable.erase(cuid);
    }
    else
    {
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
    NS_LOG_FUNCTION(GetNode()->GetId() << packet->ToString());

    PacketTagIterator ti = packet->GetPacketTagIterator();
    while (ti.HasNext())
    {
        PacketTagIterator::Item item = ti.Next();
        if (item.GetTypeId() == CybertwinCertificate::GetTypeId())
        {
            CybertwinCertificate cert;
            item.GetTag(cert);
            NS_LOG_DEBUG("Received tag:" << cert.ToString());
        }
    }

    PacketMetadata::ItemIterator i = packet->BeginItem();
    while (i.HasNext())
    {
        PacketMetadata::Item item = i.Next();
        if (!item.isFragment && item.type == PacketMetadata::Item::HEADER)
        {
            // assumes header won't be fragmented
            if (item.tid == CybertwinHeader::GetTypeId())
            {
                CybertwinHeader header;
                header.Deserialize(item.current);
                CYBERTWINID_t cuid = header.GetCybertwin();
                if (!header.isDataPacket())
                {
                    switch (header.GetCommand())
                    {
                    case HOST_CONNECT:
                        m_firewallTable[cuid] = CybertwinFirewall(header);
                        break;
                    case HOST_DISCONNECT:
                        if (m_firewallTable.find(cuid) == m_firewallTable.end())
                        {
                            return false;
                        }
                        m_firewallTable[cuid].Dispose();
                        m_firewallTable.erase(cuid);
                        break;
                    default:
                        break;
                    }
                }
                else if (m_firewallTable.find(cuid) == m_firewallTable.end() ||
                         !m_firewallTable[cuid].Handle(header))
                {
                    return false;
                }
            }
            else if (item.tid == CybertwinCert::GetTypeId())
            {
                CybertwinCert cert;
                cert.Deserialize(item.current);
                CYBERTWINID_t cuid = cert.GetCybertwinId();
                if (m_firewallTable.find(cuid) != m_firewallTable.end() &&
                    !m_firewallTable[cuid].Authenticate(cert))
                {
                    // TODO
                    return false;
                }
            }
        }
    }
    NS_LOG_DEBUG("Packet permitted");
    return true;
}

CybertwinFirewall::CybertwinFirewall()
    : m_ingressCredit(0),
      m_cuid(0),
      m_isUsrAuthRequired(false),
      m_usrCuid(0),
      m_state(NOT_STARTED)
{
    NS_LOG_FUNCTION(this);
}

CybertwinFirewall::CybertwinFirewall(const CybertwinHeader& header)
    : m_ingressCredit(0),
      m_cuid(header.GetCybertwin()),
      m_isUsrAuthRequired(false),
      m_usrCuid(0),
      m_state(NOT_STARTED)
{
    NS_LOG_FUNCTION(m_cuid);
}

bool
CybertwinFirewall::Handle(const CybertwinHeader& header)
{
    NS_LOG_FUNCTION(this << header << m_state);
    if (m_state != PERMITTED ||
        (header.GetCommand() == CYBERTWIN_SEND && header.GetCredit() < m_ingressCredit))
    {
        NS_LOG_DEBUG("Packet blocked");
        return false;
    }
    return true;
}

bool
CybertwinFirewall::Authenticate(const CybertwinCert& cert)
{
    if (!cert.GetIsCertValid())
    {
        NS_LOG_ERROR("Firewall #" << m_cuid << " receives invalid certificate: " << cert);
        return false;
    }
    m_ingressCredit = std::max(m_ingressCredit, cert.GetIngressCredit());
    if (cert.GetCybertwinId() == m_cuid)
    {
        // device cert
        m_isUsrAuthRequired = cert.GetIsUserAuthRequired();
    }
    else
    {
        m_usrCuid = cert.GetCybertwinId();
    }
    m_state = PERMITTED;
    return true;
}

void
CybertwinFirewall::Dispose()
{
    m_state = NOT_STARTED;
}

} // namespace ns3