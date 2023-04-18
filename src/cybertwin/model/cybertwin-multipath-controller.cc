#include "cybertwin-multipath-controller.h"
#include "cybertwin-tag.h"
#include "cybertwin-header.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/core-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CybertwinMultipathTransfer");

//*****************************************************************************
//*                    Cybertwin Data Transfer Server                         *
//*****************************************************************************

CybertwinDataTransferServer::CybertwinDataTransferServer():
    m_node(nullptr),
    m_localCybertwinID(0)
{
}

void
CybertwinDataTransferServer::Setup(Ptr<Node> node, CYBERTWINID_t cyberid, CYBERTWIN_INTERFACE_LIST_t ifs)
{
    m_node = node;
    m_localCybertwinID = cyberid;
    m_cybertwinIfs = ifs;
}

void
CybertwinDataTransferServer::Listen()
{
    // get cybertwin interface
    for (auto interface:m_cybertwinIfs)
    {
        NS_LOG_DEBUG("Start listen in "<<interface.first<<":"<<interface.second);
        Ptr<Socket> cyberEar = Socket::CreateSocket(m_node, TcpSocketFactory::GetTypeId());
        int ret = -1;
        InetSocketAddress inetAddress = InetSocketAddress(interface.first, interface.second);
        ret = cyberEar->Bind(inetAddress);
        if (ret < 0)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }

        cyberEar->SetAcceptCallback(
            MakeCallback(&CybertwinDataTransferServer::PathRequestCallback, this),
            MakeCallback(&CybertwinDataTransferServer::PathCreatedCallback, this));
        
        cyberEar->Listen();
    }
}

void 
CybertwinDataTransferServer::InsertCNRSCache(CYBERTWINID_t id, CYBERTWIN_INTERFACE_LIST_t ifs)
{
    CNRSCache[id] = ifs;
}

bool
CybertwinDataTransferServer::PathRequestCallback(Ptr<Socket> sock, const Address& addr)
{
    NS_LOG_FUNCTION(this << sock << addr);
    return true; //accept all TCP Connection request
}

void
CybertwinDataTransferServer::PathCreatedCallback(Ptr<Socket> sock, const Address& addr)
{
    NS_LOG_FUNCTION(this << sock << addr);

    // create new SinglePath
    NS_LOG_INFO("Server born new path.");
    SinglePath* path = new SinglePath();
    path->SetSocket(sock);
    path->SetServer(this);
    path->SetPeerAddress(addr);
    path->SetLocalKey(GenerateKey());
    path->SetLocalCybertwinID(m_localCybertwinID);
    path->SetPathState(SinglePath::SINGLE_PATH_LISTEN);
    
    path->PathListen();
}

MP_CONN_KEY_t
CybertwinDataTransferServer::GenerateKey()
{
    MP_CONN_KEY_t key;
    if (rand == nullptr)
    {
        rand = CreateObject<UniformRandomVariable>();
    }

    do
    {
        NS_LOG_WARN("gen rand val.");
       key = (MP_CONN_KEY_t)(rand->GetValue()*10000);
    }while(key == 0);

    return key;
}

void
CybertwinDataTransferServer::NewConnectionBuilt(SinglePath* path)
{
    NS_LOG_INFO("new connection built.");
    NS_ASSERT_MSG(path, "path is null.");

    //create new connection with path
    MultipathConnection* new_conn = new MultipathConnection(path);
    NS_ASSERT_MSG(new_conn, "new_conn is null.");

    //add to data server
    m_connections.insert(new_conn);
    m_connectionIDs[new_conn->GetConnID()] = new_conn;

    //aggravate connection to path
    path->SetConnection(new_conn);

    //inform application
    NS_LOG_INFO("inform application new connection created.");
    if (!m_notifyNewConnection.IsNull())
    {
        m_notifyNewConnection(new_conn);
    }
}

bool
CybertwinDataTransferServer::ValidConnectionID(MP_CONN_ID_t connid)
{
    NS_LOG_INFO("Vaildating the connectionID");
    return m_connectionIDs.find(connid) != m_connectionIDs.end();
}

/**
 * @brief A path try to join a connection when it's state changed
 * from SINGLE_PATH_LISTEN to SINGLE_PATH_CONNECTED.
*/
void 
CybertwinDataTransferServer::NewPathJoinConnection(SinglePath* path)
{
    NS_LOG_INFO("new path join connection.");
    NS_ASSERT_MSG(path, "path is null.");

    MP_CONN_ID_t connid = path->GetConnectionID();
    if (m_connectionIDs.find(connid) == m_connectionIDs.end())
    {
        NS_LOG_ERROR("Connection ["<<connid<<"] doesn't exist.");
        return ;
    }

    MultipathConnection* conn = m_connectionIDs[connid];
    path->SetConnection(conn);
    conn->AddOtherConnectPath(path);
}

void
CybertwinDataTransferServer::SetNewConnectCreatedCallback(Callback<void, MultipathConnection*> newConn)
{
    m_notifyNewConnection = newConn;
}

//*****************************************************************************
//*                     Multipath Reciving Buffer                             *
//*****************************************************************************
MpRxBuffer::MpRxBuffer():
    exceptSeqNum(0),
    size(0)
{
}

int32_t
MpRxBuffer::AddItem(DataItem *item)
{
    NS_ASSERT(item);
    NS_ASSERT(item->seqStart == exceptSeqNum);

    //enqueue data
    rxQueue.push(item->packet);
    //renew seqnum
    exceptSeqNum += item->length;
    size += item->length;
    return 0;
}

//*****************************************************************************
//*                     Multipath Connection                                  *
//*****************************************************************************
MultipathConnection::MultipathConnection()
{
    NS_LOG_FUNCTION("MultipathConnection constructor.");
    m_node = nullptr;
    m_localCyberID = 0;
    m_peerCyberID = 0;
    m_localKey = 0;
    m_connID = 0;
    m_connState = MP_CONN_INIT;
}


MultipathConnection::MultipathConnection(SinglePath* path)
{
    NS_LOG_FUNCTION("MultipathConnection constructor.");
    NS_ASSERT_MSG(path, "path is null.");
    NS_ASSERT_MSG(path->GetPathState() == SinglePath::SINGLE_PATH_CONNECTED , "path state is not connected.");
    m_node = nullptr; //currently not used
    m_localCyberID = path->GetLocalCybertwinID();
    m_peerCyberID = path->GetRemoteCybertwinID();
    m_localKey = path->GetLocalKey();
    m_connID = path->GetConnectionID();
    m_sendSeqNum = m_connID;
    m_recvSeqNum = m_connID;
    m_connState = MP_CONN_CONNECT;

    m_paths.push_back(path);
}

void
MultipathConnection::Setup(Ptr<Node> node, CYBERTWINID_t cyberid)
{
    m_node = node;
    m_localCyberID = cyberid;
    if (rand == nullptr)
    {
        rand = CreateObject<UniformRandomVariable>();
    }
    do {
        NS_LOG_INFO("gen rand val.");
        m_localKey = (MP_CONN_KEY_t)(rand->GetValue()*10000);
    }while (m_localKey == 0);

    m_connState = MP_CONN_INIT;
}

void
MultipathConnection::Connect(CYBERTWINID_t targetID)
{
    NS_LOG_INFO("Connceting to remote Cybertwin : "<<targetID);
    m_peerCyberID = targetID;
    //resolve the cyberID
    if (CNRSCache.find(targetID) != CNRSCache.end())
    {
        CYBERTWIN_INTERFACE_LIST_t interfaces = CNRSCache[targetID];
        m_pathNum = interfaces.size();
        NS_LOG_INFO("Cybertwin "<<targetID<<" contain "<<m_pathNum<<" interfaces.");
        for (int32_t i=0; i<m_pathNum; i++)
        {
            NS_LOG_INFO("Connecting No."<<i<<" interface.");
            SinglePath* path = new SinglePath();
            Ptr<Socket> sock = Socket::CreateSocket(m_node, TcpSocketFactory::GetTypeId());
            path->SetSocket(sock);
            path->SetRemoteInteface(interfaces[i]);
            path->SetLocalKey(m_localKey);
            path->SetLocalCybertwinID(m_localCyberID);
            path->SetConnection(this);

            path->PathConnect();
        }   

    }else
    {
        NS_LOG_INFO("Resolving Cybertwin ID to interfaces.");
        //CNRS 
        //Simulator::Schedule();
    }
}

/**
 * @brief Send data to remote Cybertwin
 * 
 * @param packet
 * @return int32_t 
 */
int32_t 
MultipathConnection::Send(Ptr<Packet> packet)
{
    int32_t size = packet->GetSize();
    if (size == 0)
    {
        NS_LOG_INFO("Packet size is 0.");
        return 0;
    }

    //construct header
    MultipathHeaderDSN header;
    header.SetCuid(m_localCyberID);
    header.SetDataSeqNum(m_sendSeqNum);
    m_sendSeqNum += packet->GetSize(); //update seqnum
    header.SetDataLen(packet->GetSize());

    packet->AddHeader(header);
    m_txBuffer.push(packet);
    Simulator::ScheduleNow(&MultipathConnection::SendData, this);
 
    return size;
}

void
MultipathConnection::SendData()
{
    if (m_txBuffer.empty())
    {
        NS_LOG_DEBUG("TxBuffer is empty.");
        return ;
    }


    Ptr<Packet> sendPkt = m_txBuffer.front();
    m_txBuffer.pop();

    //choose one path to send
    int32_t pathIndex = ChoosePathRoundRobin();
    if (pathIndex == -1)
    {
        NS_LOG_DEBUG("No path to send.");
        return ;
    }

    NS_LOG_DEBUG("Choose No."<<pathIndex<<" path to send.");
    SinglePath* path = m_paths[pathIndex];
    path->Send(sendPkt);
}

Ptr<Packet>
MultipathConnection::Recv()
{
    NS_LOG_INFO("Recv");
    Ptr<Packet> pack = m_rxBuffer.front();
    m_rxBuffer.pop();
    return pack;
}

void
MultipathConnection::PathRecvedData(SinglePath* path)
{
    NS_LOG_FUNCTION(this);
    //NS_LOG_DEBUG("Connection Recved Data, drain from path.");
    //get data from path

    Ptr<Packet> pack = nullptr;
    //extract data from path
    while(m_recvSeqNum == path->m_rxHeadSeqNum)
    {
        pack = path->Recv();
        if (pack == nullptr)
        {
            NS_LOG_DEBUG("Connection Recved Data, no data from path.");
            break;
        }

        //add to rx buffer
        NS_LOG_INFO("Connection Recved Data, add to rx buffer.");
        m_rxBuffer.push(pack);
        m_recvSeqNum += pack->GetSize();    //renew seqnum
    }

    //notify upper layer
    if (!m_recvCallback.IsNull())
    {
        //NS_LOG_DEBUG("Connection Recved Data, notify upper layer.");
        m_recvCallback(this);
    }
}

/**
 * @brief Close the connection
*/
int32_t
MultipathConnection::Close()
{
    NS_LOG_FUNCTION(this);
    //if rxBuffer is empty then close
    //else wait for 10ms and check again
    int closedNum = 0;
    if (m_rxBuffer.empty())
    {
        m_connState = MP_CONN_CLOSING;
        int32_t num = m_paths.size();
        for (int32_t i=0; i<num; i++)
        {
            if (m_paths[i] == nullptr)
            {
                closedNum++;
                continue;
            }

            if (m_paths[i]->m_pathState == SinglePath::SINGLE_PATH_CLOSED)
            {
                //delete path
                m_paths[i]->PathClean();
                delete m_paths[i];
            }
            
            m_paths[i]->PathClose();
        }
    }

    if (closedNum == (int32_t)m_paths.size())
    {
        NS_LOG_INFO("Connection Closed.");
        m_connState = MP_CONN_CLOSED;
        if (!m_closeCallback.IsNull())
        {
            NS_LOG_INFO("Connection Closed, notify upper layer."); 
            m_closeCallback(this);  //notify upper layer
        }
    }else
    {
        NS_LOG_INFO("Connection Closing.");
        Simulator::Schedule(MilliSeconds(10), &MultipathConnection::Close, this);
    }

    return 0;
}

void
MultipathConnection::AddRawPath(SinglePath *path, bool ready)
{
    NS_LOG_INFO("Connection try to add new path");
    NS_ASSERT(path);
    if (ready)
    {
        m_rawReadyPath.push(path);
    }else
    {
        m_rawFailPath.push(path);
    }
    if ((int32_t)(m_rawReadyPath.size() + m_rawFailPath.size()) == m_pathNum)
    {
        //start build connection
        NS_LOG_DEBUG("All "<< m_pathNum << " path initialized, start build connection.");
        Simulator::Schedule(TimeStep(1), &MultipathConnection::BuildConnection, this);
    }
}

//choose one path from the rawpath set
//and use this path to init connection
void
MultipathConnection::BuildConnection()
{
    NS_LOG_INFO("Building connection.");
    if (m_rawReadyPath.size() <= 0)
    {
        NS_LOG_ERROR("Error, no ready path exists.");
        return ;
    }
    SinglePath* path = m_rawReadyPath.front();
    m_rawReadyPath.pop();
    path->InitConnection();
}

//called by the first connected path
void 
MultipathConnection::AddInitConnectPath(SinglePath* path)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Add init connect path.");
    m_connID = path->GetConnectionID();
    NS_ASSERT(m_connID != 0);
    m_sendSeqNum = m_connID;    //init send seq num
    m_recvSeqNum = m_connID;    //init recv seq num
    m_remoteKey = path->GetRemoteKey();
    m_readyPath.push(path);
    m_paths.push_back(path);

    m_connState = MP_CONN_CONNECT;

    //connect to the other path
    Simulator::Schedule(TimeStep(1), &MultipathConnection::ConnectOtherPath, this);
    //notify the upper layer
    if (!m_connectSucceedCallback.IsNull())
    {
        m_connectSucceedCallback(this);
    }
}

void
MultipathConnection::ConnectOtherPath()
{
    NS_LOG_INFO("Connect other path.");
    while (m_rawReadyPath.size() > 0)
    {
        // aggreate each path in the raw path set to connection
        SinglePath* path = m_rawReadyPath.front();
        m_rawReadyPath.pop();
        path->SetConnectionID(m_connID);
        path->SetRemoteKey(m_remoteKey);
        path->SetLocalCybertwinID(m_localCyberID);

        path->JoinConnection();
    }
}

//called by the subsequent connected path
void
MultipathConnection::AddOtherConnectPath(SinglePath* path)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(path->GetConnectionID() == m_connID);

    m_paths.push_back(path);
    m_connState = MP_CONN_CONNECT;
}

#if 0
bool
MultipathConnection::DataArrive(SinglePath *path)
{
    NS_LOG_FUNCTION(this);
    //receive data
    while (rxBuffer->exceptSeqNum == path->GetHeadSeqNum())
    {
        DataItem* item;
        item = path->PopItem();
        NS_ASSERT(rxBuffer->AddItem(item));
    }
    //TODO: check other paht or not?
    return true;
}

SinglePath*
MultipathConnection::LookupPathByAddress(const Address& addr)
{
    for (auto path:batch_paths)
    {
        if (addr == path->GetPeerAddress())
        {
            return path;
        }
    } 

    return nullptr;
}
#endif

uint32_t
MultipathConnection::ChoosePathRoundRobin()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_paths.size() > 0);
    uint32_t idx = -1;
    do {
        idx = (m_lastPathIndex++) % m_paths.size();
        if (m_paths[idx]->GetPathState() == SinglePath::SINGLE_PATH_CONNECTED)
        {
            NS_LOG_INFO("Choose path "<<idx);
            return idx;
        }
    } while (true);

    return idx;
}

void
MultipathConnection::SetLocalKey(MP_CONN_KEY_t key)
{
    m_localKey = key;
}

void
MultipathConnection::SetRemoteKey(MP_CONN_KEY_t key)
{
    m_remoteKey = key;
}

void 
MultipathConnection::SetConnState(MP_CONN_STATE st)
{
    NS_LOG_INFO("Connection state change to "<<st);
    m_connID = st;
}

void
MultipathConnection::InitIdentity(Ptr<Node> node,
                                  CYBERTWINID_t localid,
                                  CYBERTWINID_t remoteid,
                                  MP_CONN_ID_t connid)
{
    NS_LOG_FUNCTION(this);
    m_node = node;
    m_localCyberID = localid;
    m_peerCyberID = remoteid;
    m_connID = connid;
}

void
MultipathConnection::SetNode(Ptr<Node> node)
{
    m_node = node;
}

void
MultipathConnection::SetLocalCybertwinID(CYBERTWINID_t id)
{
    m_localCyberID = id;
}

void
MultipathConnection::SetPeerCybertwinID(CYBERTWINID_t id)
{
    m_peerCyberID = id;
}

void
MultipathConnection::SetConnID(MP_CONN_ID_t id)
{
    m_connID = id;
}

MP_CONN_ID_t
MultipathConnection::GetConnID()
{
    return m_connID;
}

void 
MultipathConnection::InsertCNRSItem(CYBERTWINID_t id, CYBERTWIN_INTERFACE_LIST_t ifs)
{
    CNRSCache[id] = ifs;
}

void
MultipathConnection::SetConnectCallback(Callback<void, MultipathConnection*> succeedCb,
                                        Callback<void, MultipathConnection*> failCb)
{
    m_connectSucceedCallback = succeedCb;
    m_connectFailedCallback = failCb;
}

void
MultipathConnection::SetRecvCallback(Callback<void, MultipathConnection*> recvCb)
{
    m_recvCallback = recvCb;
}

void
MultipathConnection::SetCloseCallback(Callback<void, MultipathConnection*> closeCb)
{
    m_closeCallback = closeCb;
}

void
MultipathConnection::PathJoinResult(SinglePath* path, bool success)
{
    NS_LOG_INFO("Path join result call.");
    //TODO: a lot more things to do
    if (success)
    {
        //add path to ready set
        NS_LOG_INFO("Path successfully joined the connection");
        NS_ASSERT_MSG(path->GetConnectionID() == m_connID, "Error connection id.");
        m_readyPath.push(path);
        m_paths.push_back(path);
    }else
    {
        NS_LOG_DEBUG("Path failed to join the connection");
        m_errorPath.insert(path);
    }

    //report connection status
    if (m_readyPath.size() + m_errorPath.size() == (uint32_t)m_pathNum)
    {
        //all path init join
        NS_LOG_DEBUG("All paths have tried to join the connection.");
        if (m_readyPath.size() == (uint32_t)m_pathNum)
        {
            //all path successfully joined
            NS_LOG_DEBUG("All paths successfully joined the connection.");
        }else
        {
            //some path failed to join
            NS_LOG_DEBUG("Successfully joined "<<m_readyPath.size()<<" paths."<<m_errorPath.size()<<" paths failed to join.");
        }
    }else
    {
        NS_LOG_INFO("Still waiting for "<<m_pathNum - m_readyPath.size() - m_errorPath.size()<<" paths to join the connection.");
    }
}

//*****************************************************************************
//*                              Single Path                                  *
//*****************************************************************************

SinglePath::SinglePath():
    m_socket(nullptr),
    m_localKey(0),
    m_remoteKey(0),
    m_localCybertwinID(0),
    m_remoteCybertwinID(0),
    m_connection(nullptr),
    m_server(nullptr),
    m_pathState(SINGLE_PATH_INIT),
    m_connID(0)
{}

int32_t
SinglePath::Send(Ptr<Packet> pkt)
{
    NS_LOG_FUNCTION(this);
    if (m_socket == nullptr)
    {
        NS_LOG_ERROR("Socket not initialized.");
        return -1;
    }

    if (m_pathState != SINGLE_PATH_CONNECTED)
    {
        NS_LOG_ERROR("Path not established.");
        return -1;
    }

    return m_socket->Send(pkt);
}

Ptr<Packet>
SinglePath::Recv()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Path recv.");
    if (m_rxBuffer.empty())
    {
        NS_LOG_DEBUG("No packet in rx buffer.");
        return nullptr;
    }
    NS_LOG_INFO("Packet in rx buffer.");
    Ptr<Packet> pack = m_rxBuffer.front();
    m_rxBuffer.pop();

    return pack;
}

/**
 * @brief connect to peer endpoint to establish a path
*/
int32_t
SinglePath::PathConnect()
{
    if (m_socket == nullptr)
    {
        NS_LOG_ERROR("Socket not initialized.");
        return -1;
    }

    m_socket->Bind();
    m_socket->Connect(InetSocketAddress(m_remoteIf.first, m_remoteIf.second));

    m_socket->SetConnectCallback(MakeCallback(&SinglePath::PathConnectSucceeded, this),
                                 MakeCallback(&SinglePath::PathConnectFailed, this));
    m_socket->SetCloseCallbacks(MakeCallback(&SinglePath::PathCloseSucceeded, this),
                                MakeCallback(&SinglePath::PathCloseFailed, this));
    return 0;
}

int32_t
SinglePath::PathListen()
{
    if (m_socket == nullptr)
    {
        NS_LOG_ERROR("Socket not initialized.");
        return -1;
    }

    m_pathState = SINGLE_PATH_LISTEN;
    m_socket->SetRecvCallback(MakeCallback(&SinglePath::PathRecvHandler, this));

    return 0;
}

int32_t
SinglePath::PathClose()
{
    if (m_socket == nullptr)
    {
        NS_LOG_ERROR("Socket not initialized.");
        return -1;
    }
        
    m_socket->Close();
    return 0;
}

void
SinglePath::PathClean()
{
    //TODO: clean all the data, currently just close the socket
}

void
SinglePath::PathConnectSucceeded(Ptr<Socket> sock)
{
    NS_LOG_INFO("Path connect success.");
    //set receive handler    
    sock->SetRecvCallback(MakeCallback(&SinglePath::PathRecvHandler, this));
    //m_socket = sock;

    //change state
    m_pathState = SINGLE_PATH_READY;

    //Send join request
    m_connection->AddRawPath(this, true);
}

void
SinglePath::PathConnectFailed(Ptr<Socket> sock)
{
    NS_LOG_INFO("Path connect failed.");
    m_pathState = SINGLE_PATH_ERROR;

    m_connection->AddRawPath(this, false);
}

void
SinglePath::PathRecvHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    StateProcesser();
}

void
SinglePath::PathCloseSucceeded(Ptr<Socket> socket)
{
    NS_LOG_INFO("Path close normally.");
    m_pathState = SINGLE_PATH_CLOSED;
    StateProcesser();
}

void
SinglePath::PathCloseFailed(Ptr<Socket> socket)
{
    NS_LOG_INFO("Path close with error.");
    m_pathState = SINGLE_PATH_ERROR;
    StateProcesser();
}

//first path to build the connection
void
SinglePath::InitConnection()
{
    MultipathHeader connHeader;
    connHeader.SetPathId(m_pathId);
    connHeader.SetCuid(m_localCybertwinID);
    connHeader.SetSenderKey(m_localKey);
    connHeader.SetRecverKey(0);
    connHeader.SetConnId(0);

    SendPacketWithHeader(connHeader);

    m_pathState = SINGLE_PATH_BUILD_SENT;
}

//other path to join the connection
void
SinglePath::JoinConnection()
{
    MultipathHeader connHeader;
    connHeader.SetPathId(m_pathId);
    connHeader.SetCuid(m_localCybertwinID);
    connHeader.SetSenderKey(m_localKey);
    connHeader.SetRecverKey(m_remoteKey);
    connHeader.SetConnId(m_connID);

    SendPacketWithHeader(connHeader);

    m_pathState = SINGLE_PATH_JOIN_SENT;
}

int32_t
SinglePath::SendPacketWithHeader(MultipathHeader header)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);

    return m_socket->Send(packet);
}

void
SinglePath::StateProcesser()
{
    switch (m_pathState)
    {
    case SINGLE_PATH_INIT:
        ProcessInit();
        break;
    case SINGLE_PATH_BUILD_SENT:
        ProcessBuildSent();
        break;
    case SINGLE_PATH_LISTEN:
        ProcessListen();
        break;
    case SINGLE_PATH_JOIN_SENT:
        ProcessJoinSent();
        break;
    case SINGLE_PATH_JOIN_RCVD:
        ProcessJoinRcvd();
        break;
    case SINGLE_PATH_CONNECTED:
        ProcessConnected();
        break;
    case SINGLE_PATH_CLOSED:
        ProcessClosed();
        break;
    case SINGLE_PATH_ERROR:
        ProcessError();
        break;
    default:
        break;
    }
}

void
SinglePath::ProcessBuildSent()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("ProcessBuildSent");
    Ptr<Packet> packet;
    Address from;
    while ((packet = m_socket->RecvFrom(from)))
    {
        MultipathHeader rcvHeader;
        if (packet->PeekHeader(rcvHeader) == 0)
        {
            NS_LOG_DEBUG("Unknown packet, except multipath header.");
            continue;
        }

        MP_CONN_ID_t connID = rcvHeader.GetConnId();
        MP_CONN_KEY_t remoteKey = rcvHeader.GetSenderKey();
        //naive check
        NS_ASSERT(connID != 0 && remoteKey != 0);
        m_connID = connID;
        m_remoteKey = remoteKey;

        m_pathState = SINGLE_PATH_CONNECTED;
    }

    if (m_pathState == SINGLE_PATH_CONNECTED)
    {
        Simulator::Schedule(TimeStep(1), &MultipathConnection::AddInitConnectPath, m_connection, this); //inform connection
    }
}

void
SinglePath::ProcessInit()
{
    NS_LOG_INFO("Process Init.");
}

void
SinglePath::ProcessError()
{
    NS_LOG_INFO("Process Error.");
}

void
SinglePath:: ProcessClosed()
{
    NS_LOG_INFO("Process Closed.");
}

void
SinglePath::ProcessConnected()
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    Address from;
    while ((packet = m_socket->RecvFrom(from)))
    {
        MultipathHeaderDSN dsnHeader;
        packet->PeekHeader(dsnHeader);
        dsnHeader.Print(std::cout);

        NS_LOG_DEBUG("packet size = "<<packet->GetSize() <<" Header data len = "<<dsnHeader.GetDataLen());

        //check whether the packet Header and payload match
        NS_ASSERT_MSG(dsnHeader.GetDataLen() + dsnHeader.GetSerializedSize() == packet->GetSize(), "Path Header and Payload don't match.");

        //first packet in the buffer, renew head sequence number
        if (m_rxBuffer.empty())
        {
            m_rxHeadSeqNum = dsnHeader.GetDataSeqNum();
        }

        m_rxBuffer.push(packet);
    }

    //Notify connection
    Simulator::Schedule(TimeStep(1), &MultipathConnection::PathRecvedData, m_connection, this);
}

void
SinglePath::ProcessJoinRcvd()
{
    NS_LOG_INFO("Process JoinRcvd.");
#if 0
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    Address from;
    while ((packet = m_scoket->RecvFrom(from)))
    {
        CybertwinMpTagJoin rcvTag;
        MP_CONN_ID_t connID;
        int32_t ret = -1;
        if (!packet->PeekPacketTag(rcvTag) || rcvTag.GetKind() != CybertwinMpTag::CONN_JOIN)
        {
            NS_LOG_DEBUG("Unknown packet.");
            continue;
        }

        if ((localKey != rcvTag.GetRecverKey()) || (remoteKey != rcvTag.GetSenderKey()))
        {
            NS_LOG_ERROR("Exchanging key doesn't match.");
            continue;
        }

        m_connectionID = rcvTag.GetConnectionID();
        m_pathState = SINGLE_PATH_CONNECTED;

        //notify data transfer server
        m_pathConnected(this);
    }
#endif
}

void
SinglePath::ProcessJoinSent()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Process Join Sent.");
    Ptr<Packet> packet;
    Address from;
    while ((packet = m_socket->RecvFrom(from)))
    {
        MultipathHeader recvHeader;
        MP_CONN_ID_t connID;
        if (packet->PeekHeader(recvHeader) == 0)
        {
            NS_LOG_DEBUG("Error packet header.");
            return;
        }

        //check key
        connID = recvHeader.GetConnId();
        if (connID != m_connID)
        {
            NS_LOG_DEBUG("Error, wrong connection id.");
            Simulator::Schedule(TimeStep(1), &MultipathConnection::PathJoinResult, m_connection, this, false);
            m_pathState = SINGLE_PATH_ERROR;
            return ;
        }

        //change state
        m_pathState = SINGLE_PATH_CONNECTED;

        //Inform connection that 
        Simulator::Schedule(TimeStep(1), &MultipathConnection::PathJoinResult, m_connection, this, true);
    }
}

void
SinglePath::ProcessListen()
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    Address from;
    while ((packet = m_socket->RecvFrom(from)))
    {
        MultipathHeader rcvHeader;
        MultipathHeader rspHeader;
        MP_CONN_ID_t connID;
        if (packet->PeekHeader(rcvHeader) == 0)
        {
            NS_LOG_DEBUG("ProcessListen - Unknown packet, except conn header.");
            continue;
        }
        rcvHeader.Print(std::cout);

        //check key
        connID = rcvHeader.GetConnId();
        if (connID == 0)
        {
            //first path
            m_remoteKey = rcvHeader.GetSenderKey();
            m_remoteCybertwinID = rcvHeader.GetCuid();
            SP_KEYS_TO_CONNEID(connID, m_remoteKey, m_localKey);
            m_connID = connID; 
            NS_LOG_DEBUG("ProcessListen - First path, connID: " << connID);

            rspHeader.SetSenderKey(m_localKey);
            rspHeader.SetRecverKey(m_remoteKey);
            rspHeader.SetConnId(m_connID);

            SendPacketWithHeader(rspHeader);

            m_pathState = SINGLE_PATH_CONNECTED;
            m_server->NewConnectionBuilt(this);
            return;
        }

        // join connection
        if (m_server->ValidConnectionID(connID))
        {
            // connection exist
            NS_LOG_DEBUG("ProcessListen - Join connection, connID: " << connID);
            m_remoteKey = rcvHeader.GetSenderKey();
            m_connID = connID;

            //m_server->AddPath2Connection(this);
            m_server->NewPathJoinConnection(this);

            rspHeader.SetSenderKey(m_localKey);
            rspHeader.SetRecverKey(m_remoteKey);
            rspHeader.SetConnId(m_connID);

            SendPacketWithHeader(rspHeader);

            m_pathState = SINGLE_PATH_CONNECTED;
        }else
        {
            // connection do not exist
            NS_LOG_ERROR("Attempt to join a non-exist connection.");
            return ;
        }
    }
}

int32_t
SinglePath::SendPacketWithJoinTag(MultipathTagConn tag)
{
    Ptr<Packet> rspPacket = Create<Packet>(128);
    rspPacket->AddPacketTag(tag);
    if (m_socket->Send(rspPacket) < 0)
    {
        NS_LOG_ERROR("Response error.");
        return -1;
    }

    return 0;
}

MP_CONN_KEY_t
SinglePath::GetLocalKey()
{
    return m_localKey;
}

void 
SinglePath::SetLocalKey(MP_CONN_KEY_t key)
{
    m_localKey = key;
}

void
SinglePath::SetRemoteKey(MP_CONN_KEY_t key)
{
    m_remoteKey = key;
}

MP_CONN_KEY_t
SinglePath::GetRemoteKey()
{
    return m_remoteKey;
}

void 
SinglePath::SetRemoteInteface(CYBERTWIN_INTERFACE_t ift)
{
    m_remoteIf = ift;
}

void
SinglePath::SetSocket(Ptr<Socket> sock)
{
    m_socket = sock;
    
}

void 
SinglePath::SetServer(CybertwinDataTransferServer* server)
{
    m_server = server;
}

void
SinglePath::SetPathId(MP_PATH_ID_t id)
{
    m_pathId = id;
}

MP_PATH_ID_t
SinglePath::GetPathId()
{
    return m_pathId;
}

void
SinglePath::SetPathState(PathStatus stat)
{
    m_pathState = stat;
}

SinglePath::PathStatus
SinglePath::GetPathState()
{
    return m_pathState;
}

void
SinglePath::SetConnectionID(MP_CONN_ID_t id)
{
    m_connID = id;
}

MP_CONN_ID_t
SinglePath::GetConnectionID()
{
    return m_connID;
}

void 
SinglePath::SetPeerAddress(Address addr)
{
   m_peerAddr = addr;
}

void
SinglePath::SetConnection(MultipathConnection* conn)
{
    m_connection = conn;
}

void
SinglePath::SetLocalCybertwinID(CYBERTWINID_t id)
{
    m_localCybertwinID = id;
}

CYBERTWINID_t
SinglePath::GetLocalCybertwinID()
{
    return m_localCybertwinID;
}

void
SinglePath::SetRemoteCybertwinID(CYBERTWINID_t id)
{
    m_remoteCybertwinID = id;
}

CYBERTWINID_t
SinglePath::GetRemoteCybertwinID()
{
    return m_remoteCybertwinID;
}

