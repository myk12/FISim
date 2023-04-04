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

    void BornCybertwin(const CybertwinHeader&, Ptr<Socket>);
    void KillCybertwin(const CybertwinHeader&, Ptr<Socket>);

    Address m_localAddr;
    uint64_t m_localPort;

    Ptr<Socket> m_socket;
    std::unordered_map<CYBERTWINID_t, Ptr<Cybertwin>> m_cybertwinTable;
};

class CybertwinFirewall
{
  public:
    CybertwinFirewall();

    bool Filter(CYBERTWINID_t, const CybertwinCreditTag&);
    bool Forward(Ptr<const Packet>);

    bool Authenticate(const CybertwinCertificate&);
    void Dispose();

  private:
    enum FirewallState_t
    {
        NOT_STARTED,
        PERMITTED,
        THROTTLED,
        DENIED
    };

    uint16_t m_ingressCredit;
    CYBERTWINID_t m_cuid;
    bool m_isUsrAuthRequired;
    CYBERTWINID_t m_usrCuid;
    FirewallState_t m_state;
};

class CybertwinTrafficManager : public Application
{
  public:
    static TypeId GetTypeId();
    CybertwinTrafficManager();
    ~CybertwinTrafficManager();

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    bool InspectPacket(Ptr<NetDevice>, Ptr<const Packet>, uint16_t);
    std::unordered_map<CYBERTWINID_t, CybertwinFirewall> m_firewallTable;
};

} // namespace ns3

#endif