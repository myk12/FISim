#ifndef CYBERTWIN_EDGE_H
#define CYBERTWIN_EDGE_H

#include "cybertwin-common.h"
#include "cybertwin-packet-header.h"
#include "cybertwin.h"

#include "ns3/address.h"
#include "ns3/application.h"

#include <unordered_map>

namespace ns3
{

class CybertwinController : public Application
{
  public:
    static TypeId GetTypeId();
    CybertwinController();
    ~CybertwinController();

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    bool HostConnecting(Ptr<Socket>, const Address&);
    void HostConnected(Ptr<Socket>, const Address&);

    void NormalHostClose(Ptr<Socket>);
    void ErrorHostClose(Ptr<Socket>);

    void ReceiveFromHost(Ptr<Socket>);

    void BornCybertwin(Ptr<Socket>, const CybertwinControllerHeader&);
    void KillCybertwin(Ptr<Socket>, const CybertwinControllerHeader&);

    Address m_localAddr;
    uint64_t m_localPort;

    uint16_t m_nextLocalPort{LOCAL_PORT_COUNTER_START};
    uint16_t m_nextGlobalPort{GLOBAL_PORT_COUNTER_START};

    Ptr<Socket> m_socket;
    std::unordered_map<CYBERTWINID_t, Ptr<Cybertwin>> m_cybertwinTable;
};

void RespToHost(Ptr<Socket>, Ptr<Packet>);

} // namespace ns3

#endif