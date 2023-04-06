#ifndef CYBERTWIN_EDGE_H
#define CYBERTWIN_EDGE_H

#include "cybertwin-common.h"
#include "cybertwin-header.h"
#include "cybertwin.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/node.h"

#include <unordered_map>

namespace ns3
{

// class CybertwinCreditMonitor
// {
// };

class CybertwinFirewall : public Application
{
  public:
    static TypeId GetTypeId();
    CybertwinFirewall(CYBERTWINID_t cuid = 0);

    bool ReceiveFromGlobal(CYBERTWINID_t, const CybertwinCreditTag&);
    bool ReceiveFromLocal(Ptr<const Packet>);
    bool ReceiveCertificate(const CybertwinCertTag&);
    int Forward(CYBERTWINID_t, Ptr<Socket>, Ptr<Packet>);

  protected:
    void DoDispose() override;

  private:
    enum FirewallState_t
    {
        NOT_STARTED,
        PERMITTED,
        THROTTLED,
        DENIED
    };

    void StartApplication() override;
    void StopApplication() override;

    uint16_t m_credit;
    uint16_t m_ingressCredit;
    CYBERTWINID_t m_cuid;
    bool m_isUsrAuthRequired;
    CYBERTWINID_t m_usrCuid;
    uint16_t m_usrCredit;
    FirewallState_t m_state;
};

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

    bool InspectPacket(Ptr<NetDevice>, Ptr<const Packet> packet, uint16_t);
    void ReceiveFromHost(Ptr<Socket>);

    void CybertwinInit(Ptr<Socket>, const CybertwinHeader&);
    int CybertwinSend(CYBERTWINID_t, CYBERTWINID_t, Ptr<Socket>, Ptr<Packet>);

    Address m_localAddr;
    uint64_t m_localPort;

    Ptr<Socket> m_socket;
    std::unordered_map<CYBERTWINID_t, Ptr<Cybertwin>> m_cybertwinTable;
    std::unordered_map<CYBERTWINID_t, Ptr<CybertwinFirewall>> m_firewallTable;
};

} // namespace ns3

#endif