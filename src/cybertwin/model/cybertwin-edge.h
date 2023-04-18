#ifndef CYBERTWIN_EDGE_H
#define CYBERTWIN_EDGE_H

#include "cybertwin-common.h"
#include "cybertwin-header.h"
#include "cybertwin.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/node.h"

#include <unordered_map>
#include <unordered_set>

namespace ns3
{

// class CybertwinCreditMonitor
// {
// };
class Cybertwin;

class CybertwinFirewall : public Application
{
  public:
    static TypeId GetTypeId();
    CybertwinFirewall(CYBERTWINID_t cuid = 0);

    bool ReceiveFromGlobal(CYBERTWINID_t, const CybertwinCreditTag&);
    bool ReceiveFromLocal(Ptr<const Packet>);
    bool ReceiveCertificate(const CybertwinCertTag&);

    int ForwardToGlobal(CYBERTWINID_t, Ptr<Socket>, Ptr<Packet>);
    int ForwardToLocal(Ptr<Socket>, Ptr<Packet>);

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
    int CybertwinReceive(CYBERTWINID_t, Ptr<Socket>, Ptr<Packet>);

    void AssignInterfaces(CYBERTWIN_INTERFACE_LIST_t&);

    Address m_localAddr;
    uint64_t m_localPort;
    std::vector<Ipv4Address> m_localIpv4AddrList;

    Ptr<Socket> m_socket;
    std::unordered_map<CYBERTWINID_t, Ptr<Cybertwin>> m_cybertwinTable;
    std::unordered_map<CYBERTWINID_t, Ptr<CybertwinFirewall>> m_firewallTable;

    std::unordered_set<uint16_t> m_assignedPorts;
    uint16_t m_lastAssignedPort;
};

} // namespace ns3

#endif