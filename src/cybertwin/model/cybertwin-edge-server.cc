#include "cybertwin-edge-server.h"

#include "ns3/log.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/tcp-socket.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinEdgeServer");
NS_OBJECT_ENSURE_REGISTERED(CybertwinEdgeServer);

TypeId
CybertwinEdgeServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinEdgeServer")
                            .SetParent<Application>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinEdgeServer>()
                            .AddAttribute("LocalAddress",
                                          "The address on which to bind the listening socket",
                                          AddressValue(),
                                          MakeAddressAccessor(&CybertwinEdgeServer::m_localAddr),
                                          MakeAddressChecker())
                            .AddAttribute("LocalPort",
                                          "The port on which the application sends data",
                                          UintegerValue(443),
                                          MakeUintegerAccessor(&CybertwinEdgeServer::m_localPort),
                                          MakeUintegerChecker<uint16_t>());
    return tid;
}

CybertwinEdgeServer::CybertwinEdgeServer()
    : m_listenSocket(nullptr),
      m_controlTable(Create<CybertwinControlTable>())
{
    NS_LOG_FUNCTION(this);
}

void
CybertwinEdgeServer::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_listenSocket = nullptr;
    m_controlTable->DoDispose();

    std::unordered_map<Ptr<Socket>, Ptr<StreamState>>::iterator it;
    for (it = m_streamBuffer.begin(); it != m_streamBuffer.end(); ++it)
    {
        it->first->Close();
        it->first->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                     MakeNullCallback<void, Ptr<Socket>>());
        it->first->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    m_streamBuffer.clear();

    Application::DoDispose();
}

void
CybertwinEdgeServer::StartApplication()
{
    NS_LOG_FUNCTION(this);
    // Create the socket if not already
    if (!m_listenSocket)
    {
        m_listenSocket = Socket::CreateSocket(GetNode(), TcpSocketFactory::GetTypeId());
        int ret = -1;

        if (Ipv4Address::IsMatchingType(m_localAddr))
        {
            Ipv4Address ipv4 = Ipv4Address::ConvertFrom(m_localAddr);
            InetSocketAddress inetAddress = InetSocketAddress(ipv4, m_localPort);
            NS_LOG_DEBUG("Server bind to " << ipv4 << ":" << m_localPort);
            ret = m_listenSocket->Bind(inetAddress);
        }
        else if (Ipv6Address::IsMatchingType(m_localAddr))
        {
            Ipv6Address ipv6 = Ipv6Address::ConvertFrom(m_localAddr);
            Inet6SocketAddress inet6Address = Inet6SocketAddress(ipv6, m_localPort);
            NS_LOG_DEBUG("Server bind to " << ipv6 << ":" << m_localPort);
            ret = m_listenSocket->Bind(inet6Address);
        }

        if (ret == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }

        m_listenSocket->Listen();
        m_listenSocket->ShutdownSend();
    }

    m_listenSocket->SetAcceptCallback(
        MakeCallback(&CybertwinEdgeServer::ConnectionRequestCallback, this),
        MakeCallback(&CybertwinEdgeServer::NewConnectionCreatedCallback, this));
    m_listenSocket->SetRecvCallback(MakeCallback(&CybertwinEdgeServer::ReceivedDataCallback, this));
    m_listenSocket->SetCloseCallbacks(MakeCallback(&CybertwinEdgeServer::NormalCloseCallback, this),
                                      MakeCallback(&CybertwinEdgeServer::ErrorCloseCallback, this));
    m_listenSocket->Listen();
}

void
CybertwinEdgeServer::StopApplication()
{
    NS_LOG_FUNCTION(this);
    if (m_listenSocket)
    {
        m_listenSocket->Close();
        m_listenSocket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                          MakeNullCallback<void, Ptr<Socket>, const Address&>());
        m_listenSocket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                                          MakeNullCallback<void, Ptr<Socket>>());
        m_listenSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_listenSocket->SetSendCallback(MakeNullCallback<void, Ptr<Socket>, uint32_t>());
    }
}

bool
CybertwinEdgeServer::ConnectionRequestCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);
    return true;
}

void
CybertwinEdgeServer::NewConnectionCreatedCallback(Ptr<Socket> socket, const Address& address)
{
    NS_LOG_FUNCTION(this << socket << address);
    socket->SetRecvCallback(MakeCallback(&CybertwinEdgeServer::ReceivedDataCallback, this));
    socket->SetCloseCallbacks(MakeCallback(&CybertwinEdgeServer::NormalCloseCallback, this),
                              MakeCallback(&CybertwinEdgeServer::ErrorCloseCallback, this));
    ReceivedDataCallback(socket);
}

void
CybertwinEdgeServer::ReceivedDataCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> packet;
    Ptr<StreamState> socketStream;
    Ptr<CybertwinItem> cybertwin;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() == 0)
        {
            break;
        }

        bool firstPacket = false;
        if (m_streamBuffer.find(socket) == m_streamBuffer.end())
        {
            m_streamBuffer.insert(std::make_pair(socket, Create<StreamState>()));
        }

        socketStream = m_streamBuffer.find(socket)->second;
        if (socketStream->bytesToReceive == 0)
        {
            // new packet
            CybertwinPacketHeader header;
            packet->RemoveHeader(header);
            socketStream->update(header);
            firstPacket = true;
        }

        uint32_t contentSize = packet->GetSize();
        if (contentSize > socketStream->bytesToReceive)
        {
            NS_LOG_ERROR("The received packet is larger than expected");
            socketStream->bytesToReceive = 0;
            continue;
        }

        switch (socketStream->action)
        {
        case 1:
            if (contentSize)
            {
                NS_FATAL_ERROR("Failed to connect: handshake packet should be empty");
            }
            cybertwin = m_controlTable->Connect(socketStream->srcGuid, from, socket);
            break;
        case 2:
            cybertwin = m_controlTable->Get(socketStream->srcGuid);
            // cybertwin->SendTo(socketStream->dstGuid, packet->Copy(), firstPacket);
            break;
        case 3:
            cybertwin = m_controlTable->Get(socketStream->dstGuid);
            // cybertwin->RecvFrom(socketStream->srcGuid, packet->Copy(), firstPacket);
            break;
        default:
            NS_FATAL_ERROR("Failed to receive data: invalid command in packet");
            break;
        }

        socketStream->bytesToReceive -= contentSize;
    }
}

void
CybertwinEdgeServer::NormalCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    if (m_streamBuffer.find(socket) != m_streamBuffer.end())
    {
        Ptr<StreamState> socketStream = m_streamBuffer.find(socket)->second;
        if (socketStream->isClientSocket)
        {
            m_controlTable->Disconnect(socketStream->srcGuid);
        }
        socket->ShutdownSend();
        m_streamBuffer.erase(socket);
    }
    else
    {
        NS_LOG_ERROR("Failed to disconnect: socket not existed");
    }
    socket->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket>>(),
                              MakeNullCallback<void, Ptr<Socket>>());
    socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
}

void
CybertwinEdgeServer::ErrorCloseCallback(Ptr<Socket> socket)
{
    NS_LOG_ERROR("A socket error occurs:" << socket->GetErrno());
    // TODO: Set a timer and remove cybertwin after timeout
}

// Cybertwin Control Table
CybertwinControlTable::CybertwinControlTable()
{
}

void
CybertwinControlTable::DoDispose()
{
    m_table.clear();
}

Ptr<CybertwinItem>
CybertwinControlTable::Get(uint64_t guid)
{
    return m_table.find(guid)->second;
}

Ptr<CybertwinItem>
CybertwinControlTable::Connect(uint64_t guid, const Address& src, Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << guid << src << socket);
    Ptr<CybertwinItem> ret{nullptr};

    if (m_table.find(guid) == m_table.end())
    {
        // create a new cybertwin item and insert a new entry to the global guid routing table
        GuidTable.insert(std::make_pair(guid, src));
        ret = Create<CybertwinItem>(guid, src, socket);
    }
    else
    {
        // TODO (mobility): Copy from the existing address
    }

    return ret;
}

void
CybertwinControlTable::Disconnect(uint64_t guid)
{
    NS_LOG_FUNCTION(this << guid);
    // remove the record from the global guid table
    GuidTable.erase(guid);
    m_table.erase(guid);
}

// Cybertwin Item
CybertwinItem::CybertwinItem(uint64_t guid, const Address& src, Ptr<Socket> socket)
    : m_guid(guid),
      m_clientAddr(src),
      m_socket(socket)
{
    NS_LOG_FUNCTION(this << guid << src << socket);
}

CybertwinItem::~CybertwinItem()
{
    m_socket = nullptr;
}

void
CybertwinItem::SendToClient(Ptr<Packet> packet)
{
    // temporary implementation
    m_socket->Send(packet);
}

} // namespace ns3