#include "cybertwin.h"

#include "ns3/applications-module.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

namespace ns3
{


NS_LOG_COMPONENT_DEFINE("CybertwinServer");

TypeId
CybertwinServer::GetTypeId()
{
    static TypeId tid = TypeId("CybertwinServer")
                        .SetParent<Application>()
                        .SetGroupName("cybertwin");

    return tid;
}

CybertwinServer::CybertwinServer()
    :m_socket(nullptr),
     port(8080)
{
}

CybertwinServer::~CybertwinServer()
{
    m_socket = nullptr;
}

void CybertwinServer::Setup()
{
    node = GetNode();
    ipaddr = Ipv4Address::GetAny();
}

void CybertwinServer::StartApplication()
{
    // create a socket
    NS_LOG_INFO("Start CybertwinServer.");
    m_socket = Socket::CreateSocket(node, TypeId::LookupByName("ns3::TcpSocketFactory"));
    const InetSocketAddress inetsocket = InetSocketAddress(ipaddr, port);
    m_socket->Bind(inetsocket);
    m_socket->SetRecvCallback(MakeCallback(&CybertwinServer::RecvHandler, this));
    m_socket->SetAcceptCallback(MakeCallback(&CybertwinServer::ConnectionRequestCallback, this),
                                MakeCallback(&CybertwinServer::NewConnectionCreatedCallback, this));
    NS_LOG_INFO("Server now is listening...");
    m_socket->Listen();
}

void CybertwinServer::StopApplication()
{
    NS_LOG_INFO("CybertwinServer now stoped.");
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
CybertwinServer::RecvHandler(Ptr<Socket> socket)
{
    NS_LOG_INFO("CybertwinServer received packet.");

    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);
        //m_rxTrace(packet);
        //m_rxTraceWithAddresses(packet, from, localAddress);
        packet->Print(std::cout);
    }
}

void
CybertwinServer::NewConnectionCreatedCallback(Ptr<Socket> socket, const Address& addr)
{
    NS_LOG_INFO("TCP connection established.");
    socket->SetRecvCallback(MakeCallback(&CybertwinServer::RecvHandler, this));
}

bool 
CybertwinServer::ConnectionRequestCallback(Ptr<Socket> socket, const Address& addr)
{
    NS_LOG_INFO("TCP connection request callback.");
    return true; // Unconditionally accept the connection request.
}

CybertwinClient::CybertwinClient():
    m_socket(nullptr),
    node(nullptr)
{
}

CybertwinClient::~CybertwinClient()
{
    m_socket = nullptr;
    node = nullptr;
}

TypeId
CybertwinClient::GetTypeId()
{
    static TypeId tid = TypeId("CybertwinClient")
                        .SetParent<Application>()
                        .SetGroupName("cybertwin");

    return tid;
}

void
CybertwinClient::Setup(Ipv4Address peer_addr, uint16_t peer_port)
{
    m_peerAddr = peer_addr;
    m_peerPort = peer_port;
}

void CybertwinClient::StartApplication()
{
    // create a socket
    NS_LOG_INFO("Start CybertwinClient.");
    m_socket = Socket::CreateSocket(GetNode(), TypeId::LookupByName("ns3::TcpSocketFactory"));
    m_socket->Bind();
    m_socket->Connect(InetSocketAddress(m_peerAddr, m_peerPort));

    Ptr<Packet> p;
    const uint8_t msg[128+1] = {"Hello NS-3!"};
    p = Create<Packet>(msg, 128);
    m_socket->Send(p);

}

void CybertwinClient::StopApplication()
{
    m_socket = nullptr;
    node = nullptr;
}


}
