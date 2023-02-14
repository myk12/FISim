#include "cybertwin.h"

#include "ns3/applications-module.h"

namespace ns3
{


//NS_LOG_COMPONENT_DEFINE("CybertwinServer");

TypeId
CybertwinServer::GetTypeId()
{
    static TypeId tid = TypeId("CybertwinServer");

    return tid;
}

CybertwinServer::CybertwinServer()
    :m_socket(nullptr),
     node(nullptr), 
     port(8080)
{
}

CybertwinServer::~CybertwinServer()
{
    m_socket = nullptr;
}

void CybertwinServer::Setup(Ptr<Node> nodePtr, Ipv4Address ip)
{
    node = nodePtr;
    ipaddr = ip;
}

void CybertwinServer::StartApplication()
{
    // create a socket
    //NS_LOG_DEBUG("Start CybertwinServer.");
    m_socket = Socket::CreateSocket(node, TypeId::LookupByName("ns3::TcpSocketFactory"));
    const InetSocketAddress inetsocket = InetSocketAddress(ipaddr, port);
    m_socket->Bind(inetsocket);

    //NS_LOG_INFO("Server now is listening.");
    m_socket->Listen();
}

void CybertwinServer::StopApplication()
{
    if (m_socket)
    {
        m_socket->Close();
    }
}

}
