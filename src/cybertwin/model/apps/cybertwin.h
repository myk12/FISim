#ifndef CYBERTWIN_H
#define CYBERTWIN_H

#include "ns3/cybertwin-common.h"
#include "ns3/cybertwin-header.h"
#include "ns3/cybertwin-node.h"
#include "ns3/multipath-data-transfer-protocol.h"
#include "ns3/cybertwin-name-resolution-service.h"
#include "ns3/cybertwin-tag.h"
#include "ns3/cybertwin-app.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/callback.h"

#include <queue>
#include <string>
#include <unordered_map>
#include <fstream>
#include <iostream>

namespace ns3
{
class CybertwinEdgeServer;
class MultipathConnection;
class CybertwinDataTransferServer;
class Cybertwin : public CybertwinApp
{
  public:
    typedef uint128_t STREAMID_t;
#if MDTP_ENABLED
    typedef Callback<int, CYBERTWINID_t, MultipathConnection*, Ptr<const Packet>>
        CybertwinSendCallback;
#else
    typedef Callback<int, CYBERTWINID_t, Ptr<Socket>, Ptr<const Packet>>
        CybertwinSendCallback;
#endif

    Cybertwin();
    Cybertwin(CYBERTWINID_t,
              CYBERTWIN_INTERFACE_t l_interface,
              CYBERTWIN_INTERFACE_LIST_t g_interfaces);

    ~Cybertwin();

    static TypeId GetTypeId();
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    // locally
    void LocallyListen();
    bool LocalConnRequestCallback(Ptr<Socket>, const Address&);
    void LocalConnCreatedCallback(Ptr<Socket>, const Address&);
    void LocalNormalCloseCallback(Ptr<Socket>);
    void LocalErrorCloseCallback(Ptr<Socket>);

    void LocalRecvCallback(Ptr<Socket>);
    void LocallyForward();

    // globally
    void GloballyListen();

#if MDTP_ENABLED
    void NewMpConnectionCreatedCallback(MultipathConnection* conn);
    void NewMpConnectionErrorCallback(MultipathConnection* conn);
    void MpConnectionRecvCallback(MultipathConnection* conn);
    void MpConnectionClosedCallback(MultipathConnection* conn);

    void DownloadFileServer(MultipathConnection* conn);
#else
    bool NewSpConnectionRequestCallback(Ptr<Socket> sock);
    void NewSpConnectionCreatedCallback(Ptr<Socket> sock);
    void NewSpConnectionErrorCallback(Ptr<Socket> sock);
    void SpConnectionRecvCallback(Ptr<Socket> sock);
    void SpNormalCloseCallback(Ptr<Socket> sock);
    void SpErrorCloseCallback(Ptr<Socket> sock);
#endif

    void SendPendingPackets(STREAMID_t streamid);
    void SocketConnectWithResolvedCybertwinName(Ptr<Socket> sock,
                                                CYBERTWINID_t cyberid,
                                                CYBERTWIN_INTERFACE_LIST_t ifs);

    CybertwinSendCallback SendPacket;

    // buffer for handling packet fragments; also used for recording all accepted sockets
    std::unordered_map<Ptr<Socket>, Ptr<Packet>> m_streamBuffer;
    std::unordered_map<CYBERTWINID_t, Ptr<Socket>> m_txBuffer;

    // data transmission between cybertwins
    std::unordered_map<CYBERTWINID_t, std::queue<Ptr<Packet>>> m_txPendingBuffer;
#if MDTP_ENABLED // Multipath Connection
    std::unordered_map<CYBERTWINID_t, MultipathConnection*> m_txConnections;
    std::unordered_map<CYBERTWINID_t, MultipathConnection*> m_pendingConnections;
    std::unordered_map<CYBERTWINID_t, MultipathConnection*> m_rxConnections;
    std::unordered_map<CYBERTWINID_t, std::queue<Ptr<Packet>>> m_rxPendingBuffer;
    std::unordered_map<CYBERTWINID_t, TracedValue<uint64_t>> m_rxSizePerSecond;
#else  // Naive Socket
    //std::unordered_map<STREAMID_t, Ptr<Socket>> m_txConnections;
    std::unordered_map<Ptr<Socket>, CYBERTWINID_t> m_txConnectionsReverse;
    //std::unordered_map<CYBERTWINID_t, Ptr<Socket>> m_pendingConnections;
    //std::unordered_map<Ptr<Socket>, CYBERTWINID_t> m_pendingConnectionsReverse;
    std::unordered_set<Ptr<Socket>> m_rxConnections;
    std::unordered_map<Ptr<Socket>, Address> m_rxConnectionsReverse;
    std::unordered_map<Ptr<Socket>, std::queue<Ptr<Packet>>> m_rxPendingBuffer;
    std::unordered_map<Ptr<Socket>, TracedValue<uint64_t>> m_rxSizePerSecond;
#endif
private:
    // tx buffer
    std::unordered_map<STREAMID_t, std::queue<Ptr<Packet>>> m_txStreamBuffer;
    std::vector<STREAMID_t> m_txStreamBufferOrder;
    uint32_t m_lastTxStreamBufferOrder;

    std::unordered_map<STREAMID_t, Ptr<Socket>> m_txConnections;    //established connections
    std::unordered_map<STREAMID_t, Ptr<Socket>> m_pendingConnections;   //pending connections
    std::unordered_map<Ptr<Socket>, STREAMID_t> m_pendingConnectionsReverse;

private:
    // private member data    
    CYBERTWINID_t m_cybertwinId;
    Address m_address;

    // end related
    Ptr<Socket> m_localSocket;
    CYBERTWIN_INTERFACE_t m_localInterface;

    // cloud related
    CYBERTWIN_INTERFACE_LIST_t m_globalInterfaces;

    Ptr<NameResolutionService> m_cnrs;
    std::unordered_map<CYBERTWINID_t, CYBERTWIN_INTERFACE_LIST_t> nameResolutionCache;
    // Cybertwin multiple interfaces

#if MDTP_ENABLED
    // Cybertwin Connections
    CybertwinDataTransferServer* m_dtServer;
#else
    Ptr<Socket> m_dtServer;
#endif
    uint32_t m_noneDataCnt;

    uint64_t m_serverTxBytes; //number of sent times

    std::ofstream m_MpLogFile;
    std::string m_MpLogFileName;

    // statistic
    void CybertwinCommModelStatistical();
    // traffic shaping
    void CybertwinCommModelTrafficShaping(uint32_t speed);
    void GenerateToken(double);
    void ConsumeToken();
    void TrafficShapingStatistical();
    Time m_lastShapingTime;
    void StopTrafficShaping();
    EventId m_tokenGeneratorEvent;
    EventId m_consumerEvent;
    EventId m_statisticalEvent;
    uint64_t m_tokenBucket;
    uint64_t m_consumeBytes;
    bool m_statisticalEnd;

    // traffic policing
    void CybertwinCommModelTrafficPolicing();
    bool m_isStart;
    uint64_t m_comm_test_total_bytes;
    uint64_t m_comm_test_interval_bytes;
    Time m_startTime;
    Time m_lastTime;
    Time m_endTime;
    std::queue<Ptr<Packet>> m_tsPktQueue;

    //TODO: delete this
};

}; // namespace ns3

#endif