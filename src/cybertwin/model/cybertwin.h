#ifndef CYBERTWIN_H
#define CYBERTWIN_H

#include "cybertwin-common.h"
#include "cybertwin-header.h"
#include "cybertwin-tag.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/socket.h"

namespace ns3
{

class Cybertwin : public Application
{
  public:
    Cybertwin();
    Cybertwin(CYBERTWINID_t, Ptr<Socket>, const Address&);
    ~Cybertwin();

    static TypeId GetTypeId();

  protected:
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

    // buffer for handling packet fragments; also used for recording all accepted sockets
    std::unordered_map<Ptr<Socket>, Ptr<Packet>> m_streamBuffer;
    std::unordered_map<CYBERTWINID_t, Ptr<Socket>> m_txBuffer;

    CYBERTWINID_t m_cybertwinId;
    Ptr<Socket> m_ctrlSocket;
    Address m_address;
    Ptr<Socket> m_localSocket;
    uint16_t m_localPort;
    Ptr<Socket> m_globalSocket;
    uint16_t m_globalPort;
};

} // namespace ns3

#endif