#ifndef CYBERTWIN_H
#define CYBERTWIN_H

#include "cybertwin-common.h"
#include "cybertwin-packet-header.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/socket.h"

namespace ns3
{

class Cybertwin : public Application
{
  public:
    Cybertwin();
    Cybertwin(InitCybertwinCallback);
    ~Cybertwin();

    static TypeId GetTypeId();
    void Setup(uint64_t, const Address&, uint16_t, const Address&, uint16_t);

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    void DoInit();

    void RecvFromLocal(Ptr<Socket>);
    void ReceivePacket(Ptr<Packet>);

    bool LocalConnRequestCallback(Ptr<Socket>, const Address&);
    void LocalConnCreatedCallback(Ptr<Socket>, const Address&);
    void LocalNormalCloseCallback(Ptr<Socket>);
    void LocalErrorCloseCallback(Ptr<Socket>);

    // buffer for handling packet fragments; also used for recording all accepted sockets
    std::unordered_map<Ptr<Socket>, Ptr<Packet>> m_streamBuffer;

    CYBERTWINID_t m_cybertwinId;
    InitCybertwinCallback m_initCallback;

    Ptr<Socket> m_localSocket;
    Ptr<Socket> m_globalSocket;

    Address m_localAddr;
    Address m_globalAddr;

    uint16_t m_localPort;
    uint16_t m_globalPort;
};

} // namespace ns3

#endif