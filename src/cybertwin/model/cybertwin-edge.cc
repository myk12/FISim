#include "cybertwin-edge.h"

#include "ns3/callback.h"
#include "ns3/ipv4-header.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/uinteger.h"

#include <fstream>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinEdge");
NS_OBJECT_ENSURE_REGISTERED(CybertwinController);
NS_OBJECT_ENSURE_REGISTERED(CybertwinFirewall);

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
    : m_socket(nullptr),
      m_lastAssignedPort(2000)
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
    GetNode()->SetCybertwinFirewall(MakeCallback(&CybertwinController::InspectPacket, this));
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

bool
CybertwinController::InspectPacket(Ptr<NetDevice> device,
                                   Ptr<const Packet> packet,
                                   uint16_t protocol)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << device->GetIfIndex() << packet->ToString());
    CybertwinCreditTag creditTag;
    if (packet->PeekPacketTag(creditTag))
    {
        // packet from another cybertwin
        CYBERTWINID_t cuid = creditTag.GetPeer(), src = creditTag.GetCybertwin();
        return m_firewallTable.find(cuid) != m_firewallTable.end() &&
               m_firewallTable[cuid]->ReceiveFromGlobal(src, creditTag);
    }
    CybertwinCertTag certTag;
    if (packet->PeekPacketTag(certTag))
    {
        // certificate from host
        CYBERTWINID_t cuid = certTag.GetCybertwin();
        if (m_firewallTable.find(cuid) == m_firewallTable.end())
        {
            Ptr<CybertwinFirewall> cybertwinFirewall = Create<CybertwinFirewall>(cuid);
            cybertwinFirewall->SetStartTime(Seconds(0.0));
            GetNode()->AddApplication(cybertwinFirewall);
            m_firewallTable[cuid] = cybertwinFirewall;
        }
        if (m_firewallTable[cuid]->ReceiveCertificate(certTag))
        {
            return true;
        }
        m_firewallTable.erase(cuid);
        return false;
    }
    CybertwinTag idTag;
    if (packet->PeekPacketTag(idTag))
    {
        // packet from host
        CYBERTWINID_t cuid = idTag.GetCybertwin();
        return m_firewallTable.find(cuid) != m_firewallTable.end() &&
               m_firewallTable[cuid]->ReceiveFromLocal(packet);
    }
    // non cybertwin packet
    return true;
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

        if (header.GetCommand() == HOST_CONNECT)
        {
            CYBERTWINID_t cuid = header.GetCybertwin();
            if (m_cybertwinTable.find(cuid) == m_cybertwinTable.end())
            {
                // assign interfaces for new cybertwin
                CYBERTWIN_INTERFACE_LIST_t g_interfaces;
                AssignInterfaces(g_interfaces);

                // create new cybertwin
                Ptr<Cybertwin> cybertwin = Create<Cybertwin>(
                    cuid,
                    g_interfaces,
                    m_localAddr,
                    MakeCallback(&CybertwinController::CybertwinInit, this, socket),
                    MakeCallback(&CybertwinController::CybertwinSend, this, cuid),
                    MakeCallback(&CybertwinController::CybertwinReceive, this, cuid));

                cybertwin->SetStartTime(Seconds(0.0));
                GetNode()->AddApplication(cybertwin);
                m_cybertwinTable[cuid] = cybertwin;
            }
        }
        else if (header.GetCommand() == HOST_DISCONNECT)
        {
            CYBERTWINID_t cuid = header.GetCybertwin();
            NS_ASSERT(m_cybertwinTable.find(cuid) != m_cybertwinTable.end());
            m_cybertwinTable[cuid]->SetStopTime(Seconds(0));
            m_cybertwinTable.erase(cuid);
        }
    }
}

void
CybertwinController::CybertwinInit(Ptr<Socket> socket, const CybertwinHeader& header)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << header.GetCybertwin());
    Ptr<Packet> packet = Create<Packet>(0);
    packet->AddHeader(header);
    // TODO: check if sent successfully
    socket->Send(packet);
}

int
CybertwinController::CybertwinSend(CYBERTWINID_t cuid,
                                   CYBERTWINID_t peer,
                                   Ptr<Socket> socket,
                                   Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << cuid << peer);
    if (m_firewallTable.find(cuid) != m_firewallTable.end())
    {
        return m_firewallTable[cuid]->ForwardToGlobal(peer, socket, packet);
    }
    return -1;
}

int
CybertwinController::CybertwinReceive(CYBERTWINID_t cuid, Ptr<Socket> socket, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << cuid);
    if (m_firewallTable.find(cuid) != m_firewallTable.end())
    {
        return m_firewallTable[cuid]->ForwardToLocal(socket, packet);
    }
    return -1;
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

void
CybertwinController::AssignInterfaces(CYBERTWIN_INTERFACE_LIST_t& ifs)
{
    NS_LOG_FUNCTION(this);
    for (auto addr : m_localIpv4AddrList)
    {
        CYBERTWIN_INTERFACE_t interface;

        do
        {
            interface = std::make_pair(addr, m_lastAssignedPort++);
        } while (m_assignedPorts.find(interface.second) != m_assignedPorts.end() &&
                 m_lastAssignedPort < 65535);

        if (m_lastAssignedPort < 65535)
        {
            ifs.push_back(interface);
            m_assignedPorts.insert(interface.second);
        }
        else if (m_lastAssignedPort == 65535)
        {
            NS_FATAL_ERROR("--[Ctrl-" << GetNode()->GetId() << "]: no more interfaces available");
        }
    }
}

CybertwinFirewall::CybertwinFirewall(CYBERTWINID_t cuid,
                                     LookupCredit_cb lookupCb,
                                     UpdateCredit_cb updateCb)
    : LookupCredit(lookupCb),
      UpdateCredit(updateCb),
      m_state(NOT_STARTED),
      m_ingressCredit(0),
      m_cuid(cuid),
      m_burstBytes(0),
      m_burstLimit(5000000),
      m_flowBytes(0),
      m_flowLimit(600000000)
{
}

CybertwinFirewall::~CybertwinFirewall()
{
    NS_LOG_FUNCTION(m_cuid);
}

TypeId
CybertwinFirewall::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinFirewall")
                            .SetParent<Application>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinFirewall>();
    return tid;
}

void
CybertwinFirewall::DoDispose()
{
    m_state = NOT_STARTED;
    if (!Simulator::IsFinished())
    {
        StopApplication();
    }
    Application::DoDispose();
}

void
CybertwinFirewall::StartApplication()
{
}

void
CybertwinFirewall::StopApplication()
{
}

uint16_t
CybertwinFirewall::GetCredit() const
{
    return LookupCredit(m_cuid) + m_user ? LookupCredit(m_user) : 0;
}

bool
CybertwinFirewall::ReceiveFromGlobal(CYBERTWINID_t src, const CybertwinCreditTag& creditTag)
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
CybertwinFirewall::ReceiveFromLocal(Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(m_cuid << m_state << m_ingressCredit);
    if (m_state == PERMITTED)
    {
        // TODO: examine packet content
        Time current = Simulator::Now();
        if (m_burstTs.IsZero() || current - m_burstTs >= Seconds(1.0))
        {
            m_burstTs = current;
            NS_LOG_DEBUG("Instantaneous speed: " << m_burstBytes / 1000 << "kb/s");
            m_burstBytes = 0;
        }
        if (m_flowTs.IsZero() || current - m_flowTs >= Minutes(3))
        {
            m_flowTs = current;
            NS_LOG_DEBUG("Average speed: " << m_flowBytes / 180000 << "kb/s");
            m_flowBytes = 0;
        }
        uint32_t packetSize = packet->GetSize();
        if (packetSize + m_burstBytes <= m_burstLimit && packetSize + m_flowBytes <= m_flowLimit)
        {
            m_burstBytes += packetSize;
            m_flowBytes += packetSize;
            return true;
        }
    }
    NS_LOG_DEBUG("Local packet from" << m_cuid << " got blocked");
    return false;
}

bool
CybertwinFirewall::ReceiveCertificate(const CybertwinCertTag& cert)
{
    if (cert.GetIsValid() && cert.GetCybertwin() == m_cuid)
    {
        // would return 0 if there's no user anyway
        m_user = cert.GetUser();
        m_ingressCredit = std::max(m_ingressCredit, cert.GetIngressCredit());
        m_state = PERMITTED;
        return true;
    }
    // failed to authenticate, deduct 10% of device's and user's credit score
    UpdateCredit(m_cuid, -10);
    UpdateCredit(m_user, -10);
    return false;
}

int
CybertwinFirewall::ForwardToGlobal(CYBERTWINID_t peer, Ptr<Socket> socket, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(m_cuid << peer);
    CybertwinCreditTag creditTag(m_cuid, GetCredit(), peer);
    packet->AddPacketTag(creditTag);
    return socket->Send(packet);
}

int
CybertwinFirewall::ForwardToLocal(Ptr<Socket> socket, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(m_cuid);
    // CybertwinCertTag certTag(m_cuid,
    //                          m_credit,
    //                          m_ingressCredit,
    //                          m_isUsrAuthRequired,
    //                          m_state != NOT_STARTED,
    //                          m_usrCuid,
    //                          m_usrCredit);
    // packet->AddPacketTag(certTag);
    // return socket->Send(packet);
    return 0;
}

} // namespace ns3