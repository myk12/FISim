#ifndef CYBERTWIN_H
#define CYBERTWIN_H

#include "../helper/cybertwin-node-edgeserver.h"
#include "cybertwin-common.h"
#include "cybertwin-header.h"
#include "cybertwin-multipath-controller.h"
#include "cybertwin-name-resolution-service.h"
#include "cybertwin-tag.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/callback.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-address.h"
#include "ns3/network-module.h"
#include "ns3/socket.h"

#include <queue>
#include <string>
#include <unordered_map>

namespace ns3
{
class CybertwinEdgeServer;

class Cybertwin : public Application
{
  public:
    typedef Callback<void, CybertwinHeader> CybertwinInitCallback;
    typedef Callback<int, CYBERTWINID_t, Ptr<Socket>, Ptr<const Packet>> CybertwinSendCallback;
    typedef Callback<int, Ptr<Socket>, Ptr<const Packet>> CybertwinReceiveCallback;
    typedef Callback<bool, CYBERTWINID_t, CYBERTWIN_INTERFACE_LIST_t> CybertwinUpdateCNRSCallback;

    Cybertwin();
    Cybertwin(CYBERTWINID_t,
              CYBERTWIN_INTERFACE_LIST_t g_interfaces,
              const Address&,
              CybertwinInitCallback,
              CybertwinSendCallback,
              CybertwinReceiveCallback,
              CybertwinUpdateCNRSCallback);
    ~Cybertwin();

    static TypeId GetTypeId();
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    void Initialize();

    void RecvFromSocket(Ptr<Socket>);
    void RecvLocalPacket(const CybertwinHeader&, Ptr<Packet>);
    void RecvGlobalPacket(const CybertwinHeader&, Ptr<Packet>);

    bool LocalConnRequestCallback(Ptr<Socket>, const Address&);
    void LocalConnCreatedCallback(Ptr<Socket>, const Address&);
    void LocalNormalCloseCallback(Ptr<Socket>);
    void LocalErrorCloseCallback(Ptr<Socket>);

    bool GlobalConnRequestCallback(Ptr<Socket>, const Address&);
    void GlobalConnCreatedCallback(Ptr<Socket>, const Address&);
    void GlobalNormalCloseCallback(Ptr<Socket>);
    void GlobalErrorCloseCallback(Ptr<Socket>);

    void NewMpConnectionCreatedCallback(MultipathConnection* conn);

    CybertwinInitCallback InitCybertwin;
    CybertwinSendCallback SendPacket;
    CybertwinReceiveCallback ReceivePacket;
    CybertwinUpdateCNRSCallback UpdateCNRS;

    // buffer for handling packet fragments; also used for recording all accepted sockets
    std::unordered_map<Ptr<Socket>, Ptr<Packet>> m_streamBuffer;
    std::unordered_map<CYBERTWINID_t, Ptr<Socket>> m_txBuffer;

    CYBERTWINID_t m_cybertwinId;
    Address m_address;
    Ptr<Socket> m_localSocket;
    uint16_t m_localPort;
    Ptr<Socket> m_globalSocket;
    uint16_t m_globalPort;

    std::unordered_map<CYBERTWINID_t, CYBERTWIN_INTERFACE_LIST_t> nameResolutionCache;

    // Cybretwin multiple interfaces
    CYBERTWIN_INTERFACE_LIST_t m_interfaces;
    // Cybertwin Connections
    CybertwinDataTransferServer* m_dtServer;
};
}; // namespace ns3

#endif