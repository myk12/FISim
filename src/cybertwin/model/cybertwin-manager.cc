#include "cybertwin-manager.h"

#include "ns3/callback.h"
#include "ns3/ipv4-header.h"
#include "ns3/simulator.h"
#include "ns3/tcp-header.h"
#include "ns3/uinteger.h"

#include <fstream>

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinManager");
NS_OBJECT_ENSURE_REGISTERED(CybertwinManager);

TypeId
CybertwinManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinManager")
                            .SetParent<Application>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinManager>()
                            .AddAttribute("LocalAddress",
                                          "The address on which to bind the listening socket",
                                          AddressValue(),
                                          MakeAddressAccessor(&CybertwinManager::m_localAddr),
                                          MakeAddressChecker())
                            .AddAttribute("LocalPort",
                                          "The port on which the application sends data",
                                          UintegerValue(443),
                                          MakeUintegerAccessor(&CybertwinManager::m_localPort),
                                          MakeUintegerChecker<uint16_t>())
                            .AddAttribute("ProxyPort",
                                          "The port on which the proxy listens",
                                          UintegerValue(CYBERTWIN_MANAGER_PROXY_PORT),
                                          MakeUintegerAccessor(&CybertwinManager::m_proxyPort),
                                          MakeUintegerChecker<uint16_t>());
    return tid;
}

CybertwinManager::CybertwinManager()
    : m_proxySocket(nullptr),
      m_lastAssignedPort(1000)
{
    NS_LOG_FUNCTION(this);
}

CybertwinManager::~CybertwinManager()
{
}

void
CybertwinManager::DoDispose()
{
    NS_LOG_FUNCTION(GetNode()->GetId());
    if (!Simulator::IsFinished())
    {
        // Close sockets
        StopApplication();
    }
    m_proxySocket = nullptr;
    m_cybertwinTable.clear();
    Application::DoDispose();
}

void
CybertwinManager::StartApplication()
{
    NS_LOG_FUNCTION(GetNode()->GetId());
    NS_LOG_INFO("CybertwinManager started at " << m_localAddr << ":" << m_localPort);

    Ptr<Ipv4L3Protocol> ipv4 = GetNode()->GetObject<Ipv4L3Protocol>();
    for (uint32_t i = 0; i < ipv4->GetNInterfaces(); i++)
    {
        Ptr<Ipv4Interface> ipv4If = ipv4->GetInterface(i);
        Ipv4Address ifaddr = ipv4If->GetAddress(0).GetAddress();

        NS_LOG_UNCOND("Checkt address : "<<ifaddr);
        if (ifaddr == Ipv4Address::GetAny() ||
            ifaddr == Ipv4Address::GetLoopback() ||
            ifaddr == m_localAddr)
        {
            NS_LOG_UNCOND("Skip this address.");
            continue;
        }
        NS_LOG_UNCOND("record this address.");
        m_globalIpv4AddrList.push_back(ifaddr);
        m_globalIpv4IfList.push_back(ipv4If);
    }

    StartProxy();
}

void
CybertwinManager::StartProxy()
{
    NS_LOG_FUNCTION(GetNode()->GetId());
    if (!m_proxySocket)
    {
        m_proxySocket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        //TODO: the address to be bind should be clearly specified in the future
        InetSocketAddress inetAddr = InetSocketAddress(Ipv4Address::GetAny() , m_proxyPort);
        if (m_proxySocket->Bind(inetAddr) < 0)
        {
            NS_LOG_ERROR("Failed to bind proxy socket");
        }
    }
    m_proxySocket->SetAcceptCallback(MakeCallback(&CybertwinManager::HostConnecting, this),
                                     MakeCallback(&CybertwinManager::HostConnected, this));
    m_proxySocket->SetCloseCallbacks(MakeCallback(&CybertwinManager::NormalHostClose, this),
                                    MakeCallback(&CybertwinManager::ErrorHostClose, this));
    m_proxySocket->Listen();
    NS_LOG_INFO("Proxy started at " << m_localAddr << ":" << m_proxyPort);
}

void
CybertwinManager::StopApplication()
{
    NS_LOG_FUNCTION(GetNode()->GetId());
    if (m_proxySocket)
    {
        m_proxySocket->Close();
        m_proxySocket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                    MakeNullCallback<void, Ptr<Socket>, const Address&>());
        m_proxySocket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                    MakeNullCallback<void, Ptr<Socket>>());
        m_proxySocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_proxySocket->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
    }
}

bool
CybertwinManager::HostConnecting(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket << address);
    // TODO: check if the host is malicious
    return true;
}

void
CybertwinManager::HostConnected(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket << address);

    socket->SetRecvCallback(MakeCallback(&CybertwinManager::ReceiveFromHost, this));
    socket->SetCloseCallbacks(MakeCallback(&CybertwinManager::NormalHostClose, this),
                              MakeCallback(&CybertwinManager::ErrorHostClose, this));
}

void
CybertwinManager::ReceiveFromHost(Ptr<Socket> socket)
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

        CybertwinManagerHeader header;
        packet->PeekHeader(header);

        switch (header.GetCommand())
        {
        case CYBERTWIN_REGISTRATION:
            NS_LOG_DEBUG("CYBERTWIN_REGISTRATION");
            HandleCybertwinRegistration(socket, packet);
            break;
        case CYBERTWIN_DESTRUCTION:
            NS_LOG_DEBUG("CYBERTWIN_DESTRUCTION");
            HandleCybertwinDestruction(socket, packet);
            break;
        case CYBERTWIN_RECONNECT:
            NS_LOG_DEBUG("CYBERTWIN_RECONNECT");
            HandleCybertwinReconnect(socket, packet);
            break;
        }
    }
}

void
CybertwinManager::HandleCybertwinRegistration(Ptr<Socket> socket,
                                                 Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket);
    CybertwinManagerHeader header;
    CybertwinManagerHeader replyHeader;
    std::string name;
    CYBERTWINID_t cuid;

    packet->RemoveHeader(header);

    name = header.GetCName();
    cuid = StringToUint64(name);

    // create a new cybertwin
    if (m_cybertwinTable.find(cuid) != m_cybertwinTable.end())
    {
        NS_LOG_ERROR("--[Ctrl-" << GetNode()->GetId() << "]: cybertwin already exists");

        // set reply header
        replyHeader.SetCommand(CYBERTWIN_REGISTRATION_ERROR);
        replyHeader.SetCName(name);
    }
    else
    {
        // assign local port for new cybertwin
        uint16_t port = m_lastAssignedPort++;

        // assign interfaces for new cybertwin
        CYBERTWIN_INTERFACE_LIST_t g_interfaces;
        AssignInterfaces(g_interfaces);

        // create a new cybertwin
        Ptr<Cybertwin> cybertwin = CreateObject<Cybertwin>(cuid, port, g_interfaces);
        GetNode()->AddApplication(cybertwin);
        m_cybertwinTable[cuid] = cybertwin;

        // Not started right away
        cybertwin->SetStartTime(Seconds(0.0));

        // set reply header
        replyHeader.SetCommand(CYBERTWIN_REGISTRATION_ACK);
        replyHeader.SetCName(name);
        replyHeader.SetPort(port);
    }

    // send reply
    Ptr<Packet> replyPacket = Create<Packet>(0);
    replyPacket->AddHeader(replyHeader);
    socket->Send(replyPacket);
}

void
CybertwinManager::HandleCybertwinDestruction(Ptr<Socket> socket,
                                                Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket);
    CybertwinManagerHeader header;
    CybertwinManagerHeader replyHeader;
    std::string name;
    CYBERTWINID_t cuid;

    packet->RemoveHeader(header);

    name = header.GetCName();
    cuid = StringToUint64(name);

    // destroy a cybertwin
    if (m_cybertwinTable.find(cuid) == m_cybertwinTable.end())
    {
        NS_LOG_ERROR("--[Ctrl-" << GetNode()->GetId() << "]: cybertwin does not exist");

        // set reply header
        replyHeader.SetCommand(CYBERTWIN_DESTRUCTION_ERROR);
        replyHeader.SetCName(name);
    }
    else
    {
        // destroy a cybertwin
        m_cybertwinTable.erase(cuid);

        // set reply header
        replyHeader.SetCommand(CYBERTWIN_DESTRUCTION_ACK);
        replyHeader.SetCName(name);
    }

    // send reply
    Ptr<Packet> replyPacket = Create<Packet>(0);
    replyPacket->AddHeader(replyHeader);
    socket->Send(replyPacket);
}

//TODO: Cybertwin Migration related
void
CybertwinManager::HandleCybertwinReconnect(Ptr<Socket> socket,
                                            Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket);
    CybertwinManagerHeader header;
    CybertwinManagerHeader replyHeader;
    std::string name;
    CYBERTWINID_t cuid;

    packet->RemoveHeader(header);

    name = header.GetCName();
    cuid = StringToUint64(name);

    // connect to a cybertwin
    if (m_cybertwinTable.find(cuid) == m_cybertwinTable.end())
    {
        NS_LOG_ERROR("--[Ctrl-" << GetNode()->GetId() << "]: cybertwin does not exist");

        // set reply header
        replyHeader.SetCommand(CYBERTWIN_RECONNECT_ERROR);
        replyHeader.SetCName(name);
    }
    else
    {
        // connect to a cybertwin
        //m_cybertwinTable[cuid]->Connect();

        // set reply header
        replyHeader.SetCommand(CYBERTWIN_RECONNECT_ACK);
        replyHeader.SetCName(name);
    }

    // send reply
    Ptr<Packet> replyPacket = Create<Packet>(0);
    replyPacket->AddHeader(replyHeader);
    socket->Send(replyPacket);
}

void
CybertwinManager::NormalHostClose(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << socket);
    if (socket != m_proxySocket)
    {
        socket->ShutdownSend();
    }
}

void
CybertwinManager::ErrorHostClose(Ptr<Socket> socket)
{
    NS_LOG_ERROR("--[Ctrl-" << GetNode()->GetId()
                            << "]: a socket error occurs:" << socket->GetErrno());
}

void
CybertwinManager::AssignInterfaces(CYBERTWIN_INTERFACE_LIST_t& ifs)
{
    NS_LOG_FUNCTION(GetNode()->GetId() << ifs);
    for (auto addr : m_globalIpv4AddrList)
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

} // namespace ns3