#ifndef CYBERTWIN_MULTIPATH_CONTROLLER_H
#define CYBERTWIN_MULTIPATH_CONTROLLER_H
#include "cybertwin-common.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"
#include "cybertwin-tag.h"
#include "cybertwin-header.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>

using namespace ns3;

typedef uint64_t CYBERTWIN_MPCONN_ID;
typedef std::vector<Ptr<Socket>> CYBERTWIN_SOCKS; 
struct DataItem
    {
        SinglePathSeqNum seqStart;
        Ptr<Packet> packet;
        uint32_t length;
    };

class SinglePath;
class MultipathConnection;
class CybertwinDataTransferServer;


//*****************************************************************************
//*                    Cybertwin Data Transfer Server                         *
//*****************************************************************************
// Multipath Control Block
// This class manage all the multipath connections with remote Cybertwins
class CybertwinDataTransferServer
{
public:
    CybertwinDataTransferServer();
    void Setup(Ptr<Node> node, CYBERTWINID_t cyberid, CYBERTWIN_INTERFACE_LIST_t ifs);
    void Listen();

    void InsertCNRSCache(CYBERTWINID_t , CYBERTWIN_INTERFACE_LIST_t );

    void NewConnectionCreatedCallback(SinglePath *path);
    bool ValidConnectionID(MP_CONN_ID_t connid);
    void AddPath2Connection(SinglePath* path);

    void SetNewConnectCreatedCallback(Callback<void, MultipathConnection*> newConnCb);

    // path management
private:
    // callbacks
    bool PathRequestCallback(Ptr<Socket> scok, const Address &addr);
    void PathCreatedCallback(Ptr<Socket> sock, const Address &addr);

    MP_CONN_KEY_t GenerateKey();

    // callbacks
    Callback<void, MultipathConnection*> m_notifyNewConnection;

    // identify
    Ptr<Node> m_node;
    CYBERTWINID_t m_localCybertwinID;
    CYBERTWIN_INTERFACE_LIST_t m_cybertwinIfs;

    // cybertwinID and Interfaces map
    std::unordered_map<CYBERTWINID_t, CYBERTWIN_INTERFACE_LIST_t> CNRSCache;
 
    Ptr<UniformRandomVariable> rand;
    std::unordered_set<MP_CONN_KEY_t> localKeys;

    // connections
    //std::unordered_map<SP_CONNID_t, MultipathConnection*> mpConnections
    std::unordered_set<MultipathConnection*> m_connections;
    std::unordered_map<MP_CONN_ID_t, MultipathConnection*> m_connectionIDs;
};

//*****************************************************************************
//*                     Multipath Connection                                  *
//*****************************************************************************
class MpRxBuffer
{
public:
    MpRxBuffer();
    int32_t AddItem(DataItem *item);
    SinglePathSeqNum exceptSeqNum;
    std::queue<Ptr<Packet>> rxQueue;
    uint64_t size;
};

class MultipathConnection
{
public:
    MultipathConnection();

    enum MP_CONN_STATE
    {
        MP_CONN_INIT,
        MP_CONN_BIND,
        MP_CONN_SEND,
        MP_CONN_CONNECT
    };

    Ptr<Packet> Recv();
    //int32_t Send();
    //int32_t Listen();
    void Setup(Ptr<Node> node, CYBERTWINID_t localCybertwinID);
    void Connect(CYBERTWINID_t cyberid);

    //member accessor
    void InitIdentity(Ptr<Node> node,
                      CYBERTWINID_t localid,
                      CYBERTWINID_t remoteid,
                      MP_CONN_ID_t connid);

    void InitConnection(CYBERTWINID_t localid,
                        CYBERTWINID_t remoteid,
                        MP_CONN_ID_t connid);
    void SetNode(Ptr<Node> node);
    void SetLocalKey(MP_CONN_KEY_t key);
    void SetRemoteKey(MP_CONN_KEY_t key);
    void SetLocalCybertwinID(CYBERTWINID_t id);
    void SetPeerCybertwinID(CYBERTWINID_t id);
    void SetConnID(MP_CONN_ID_t id);
    void SetConnState(MP_CONN_STATE state);

    //path manangement
    void AddRawPath(SinglePath* path, bool ready);

    //callback
    void SetConnectCallback(Callback<void, MultipathConnection*> succeedCb,
                            Callback<void, MultipathConnection*> failCb);
    void SetRecvCallback(Callback<void, MultipathConnection*> recvCb);

    //memeber accesser
    void AddInitConnectPath(SinglePath* path);
    void AddOtherConnectPath(SinglePath* path);
    void BuildConnection();
    void ConnectOtherPath();

    void PathJoinResult(SinglePath* path, bool success);

    void InsertCNRSItem(CYBERTWINID_t id, CYBERTWIN_INTERFACE_LIST_t ifs);
    //
    //MultipathConnection* GetConnFromLocalKey(MP_CONN_KEY_t key);

    MP_CONN_ID_t GetConnectionID();

private:
    MP_CONN_ID_t m_connID;
    MP_CONN_STATE m_connState;
    Ptr<Node> m_node;
    MP_CONN_KEY_t m_localKey;
    MP_CONN_KEY_t m_remoteKey;
    CYBERTWINID_t m_localCyberID;
    CYBERTWINID_t m_peerCyberID;

    MpRxBuffer* rxBuffer;
    std::queue<Ptr<Packet>> txBuffer;

    std::unordered_map<CYBERTWINID_t, CYBERTWIN_INTERFACE_LIST_t> CNRSCache;
    //path queue
    std::queue<SinglePath*> m_readyPath;

    int32_t m_pathNum;
    std::queue<SinglePath*> m_rawReadyPath;
    std::queue<SinglePath*> m_rawFailPath;
    
    std::unordered_map<MP_CONN_KEY_t, MultipathConnection*> key2ConnMap;
    std::unordered_set<MP_CONN_KEY_t> keySet;
    Ptr<UniformRandomVariable> rand;

    //callback
    Callback<void, MultipathConnection*> m_connectSucceedCallback;
    Callback<void, MultipathConnection*> m_connectFailedCallback;
    Callback<void, MultipathConnection*> m_recvCallback;
};

//*****************************************************************************
//*                              Single Path                                  *
//*****************************************************************************
class SinglePath
{
public:
    enum PathStatus
    {
        SINGLE_PATH_INIT,
        SINGLE_PATH_READY,
        SINGLE_PATH_LISTEN,
        SINGLE_PATH_BUILD_SENT,
        SINGLE_PATH_BUILD_RCVD,
        SINGLE_PATH_JOIN_SENT,
        SINGLE_PATH_JOIN_RCVD,
        SINGLE_PATH_CONNECTED,
        SINGLE_PATH_CLOSED,
        SINGLE_PATH_ERROR
    };
    SinglePath();

    //path management
    int32_t PathConnect();
    int32_t PathListen();

    //socket processer
    void PathRecvHandler(Ptr<Socket> socket);
    void SPNormalCloseHandler(Ptr<Socket> socket);
    void SPErrorCloseHandler(Ptr<Socket> socket);

    void PathConnectSucceeded(Ptr<Socket> sock);
    void PathConnectFailed(Ptr<Socket> sock);

    // the first path to build connection
    void InitConnection();
    void JoinConnection();

    //state machine
    void StateProcesser();
    void ProcessInit();
    void ProcessBuildSent();
    void ProcessListen();
    void ProcessJoinSent();
    void ProcessJoinRcvd();
    void ProcessConnected();
    void ProcessClosed();
    void ProcessError();

    //member accessor
    void SetPathId(MP_PATH_ID_t id);
    MP_PATH_ID_t GetPathId();
    void SetSocket(Ptr<Socket> sock);
    void SetServer(CybertwinDataTransferServer* server);
    //MP_CONN_KEY_t GetLocalKey();
    void SetLocalKey(MP_CONN_KEY_t key);
    MP_CONN_KEY_t GetRemoteKey();
    void SetRemoteKey(MP_CONN_KEY_t key);
    void SetRemoteInteface(CYBERTWIN_INTERFACE_t remotIf);
    void SetPeerAddress(Address addr);
    void SetConnection(MultipathConnection* conn);
    void SetPathState(PathStatus stat);
    PathStatus GetPathState();
    void SetConnectionID(MP_CONN_ID_t id);
    MP_CONN_ID_t GetConnectionID();
    void SetLocalCybertwinID(CYBERTWINID_t id);
    CYBERTWINID_t GetLocalCybertwinID();
    void SetRemoteCybertwinID(CYBERTWINID_t id);
    CYBERTWINID_t GetRemoteCybertwinID();

    //
    int32_t SendPacketWithJoinTag(MultipathTagConn tag);
    //int32_t SendPacketWithDataTag(MultipathTagConn tag);
    int32_t SendPacketWithHeader(MultipathHeader header);

    //void pullItem();
    
private:
    MP_PATH_ID_t m_pathId;
    Ptr<Socket> m_socket;
    MP_CONN_KEY_t m_localKey;
    MP_CONN_KEY_t m_remoteKey;
    CYBERTWINID_t m_localCybertwinID;
    CYBERTWIN_INTERFACE_t m_remoteIf;
    CYBERTWINID_t m_remoteCybertwinID;

    Address m_peerAddr;
    MultipathConnection *m_connection;
    CybertwinDataTransferServer *m_server;
    PathStatus m_pathState;
    MP_CONN_ID_t m_connID;

    SinglePathSeqNum nextExceptSeqNum;  //next sequence number except to receive
    SinglePathSeqNum nextSendSeqNum;    //next send sequence number

    //data buffer
    std::queue<DataItem*> rxQueue;
    std::queue<DataItem*> txQueue;
};

#endif
