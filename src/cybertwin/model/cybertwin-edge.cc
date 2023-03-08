#include "cybertwin-edge.h"

#include "ns3/callback.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinEdge");
NS_OBJECT_ENSURE_REGISTERED(CybertwinController);

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
    NS_LOG_FUNCTION(this);
}

CybertwinController::~CybertwinController()
{
    NS_LOG_FUNCTION(this);
}

void
CybertwinController::DoDispose()
{
    NS_LOG_FUNCTION(this);
    if (!Simulator::IsFinished())
    {
        StopApplication();
    }
    m_socket = nullptr;
    m_cybertwinTable.clear();
    Application::DoDispose();
}

void
CybertwinController::StartApplication()
{
    NS_LOG_FUNCTION(this);
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
        DoSocketMethod(&Socket::Bind, m_socket, m_localAddr, m_localPort);
    }
    // m_socket->SetRecvCallback(MakeCallback(&CybertwinController::ReceiveFromHost, this));
    m_socket->SetAcceptCallback(MakeCallback(&CybertwinController::HostConnecting, this),
                                MakeCallback(&CybertwinController::HostConnected, this));
    m_socket->SetCloseCallbacks(MakeCallback(&CybertwinController::NormalHostClose, this),
                                MakeCallback(&CybertwinController::ErrorHostClose, this));
    m_socket->ShutdownSend();
    m_socket->Listen();
}

void
CybertwinController::StopApplication()
{
    NS_LOG_FUNCTION(this);
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
    NS_LOG_FUNCTION(this << socket << address);
    return true;
}

void
CybertwinController::HostConnected(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);
    socket->SetRecvCallback(MakeCallback(&CybertwinController::ReceiveFromHost, this));
    socket->SetCloseCallbacks(MakeCallback(&CybertwinController::NormalHostClose, this),
                              MakeCallback(&CybertwinController::ErrorHostClose, this));
    ReceiveFromHost(socket);
}

void
CybertwinController::ReceiveFromHost(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            break;
        }

        CybertwinControllerHeader reqHeader;
        packet->PeekHeader(reqHeader);

        switch (reqHeader.GetMethod())
        {
        case NOTHING:
            NS_LOG_DEBUG("--[Edge-Ctrl]: Do nothing");
            break;
        case CYBERTWIN_CREATE:
            BornCybertwin(socket, reqHeader);
            NS_LOG_DEBUG("--[Edge-Ctrl]: Created a new cybertwin");
            break;
        case CYBERTWIN_REMOVE:
            KillCybertwin(socket, reqHeader);
            NS_LOG_DEBUG("--[Edge-Ctrl]: Removed a cybertwin");
            break;
        default:
            NS_LOG_DEBUG("--[Edge-Ctrl]: Unknown Command");
            break;
        }
    }
}

void
CybertwinController::BornCybertwin(Ptr<Socket> socket, const CybertwinControllerHeader& reqHeader)
{
    NS_LOG_FUNCTION(this << socket << reqHeader);
    // TODO: check port to prevent port conflict.
    uint16_t localPort = ++m_nextLocalPort;
    uint16_t globalPort = ++m_nextGlobalPort;

    DEVNAME_t devName = reqHeader.GetDeviceName();
    NETTYPE_t netType = reqHeader.GetNetworkType();
    // TODO: A more fancy way to generate CybertwinID
    CYBERTWINID_t cuid = devName + (netType << 8);

    if (m_cybertwinTable.find(cuid) != m_cybertwinTable.end())
    {
        // TODO: Should return a rspHeader with information related to the existed cybertwin
        NS_FATAL_ERROR("--[Edge-Ctrl]: TODO, Cybertwin Existed");
    }

    CybertwinControllerHeader rspHeader;
    rspHeader.SetCybertwinID(cuid);
    rspHeader.SetCybertwinPort(localPort);
    rspHeader.SetMethod(CYBERTWIN_CONTROLLER_SUCCESS);

    Ptr<Packet> rspPacket = Create<Packet>(0);
    rspPacket->AddHeader(rspHeader);

    Ptr<Cybertwin> cybertwin = Create<Cybertwin>();
    cybertwin->Setup(cuid, m_localAddr, localPort, m_localAddr, globalPort);
    m_cybertwinTable[cuid] = cybertwin;

    // by default, the application starts running right away
    GetNode()->AddApplication(cybertwin);
    // respond immediately, let client check if the cybertwin is generated successfully, otherwise
    // the client stops receiving packets somehow
    socket->Send(rspPacket);
}

void
CybertwinController::KillCybertwin(Ptr<Socket> socket, const CybertwinControllerHeader& reqHeader)
{
    NS_LOG_FUNCTION(this << socket << reqHeader);

    CYBERTWINID_t cuid = reqHeader.GetCybertwinID();
    if (m_cybertwinTable.find(cuid) != m_cybertwinTable.end())
    {
        Ptr<Cybertwin> cybertwin = m_cybertwinTable[cuid];
        cybertwin->SetStopTime(Seconds(0));
        m_cybertwinTable.erase(cuid);
    }
    else
    {
        NS_LOG_ERROR("--[Edge-Ctrl]: CybertwinID not found");
    }
}

void
CybertwinController::NormalHostClose(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    if (socket != m_socket)
    {
        socket->ShutdownSend();
    }
}

void
CybertwinController::ErrorHostClose(Ptr<Socket> socket)
{
    NS_LOG_ERROR("--[Edge-Ctrl]: A socket error occurs:" << socket->GetErrno());
}

void
RespToHost(Ptr<Socket> socket, Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(socket << packet);
    socket->Send(packet);
}

} // namespace ns3