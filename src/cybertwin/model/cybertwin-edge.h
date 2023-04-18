#ifndef CYBERTWIN_EDGE_H
#define CYBERTWIN_EDGE_H

#include "cybertwin-common.h"
#include "cybertwin-header.h"
#include "cybertwin.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/node.h"

// #include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
// using json = nlohmann::json;
#include <unordered_set>

namespace ns3
{

class CybertwinAssetManager : public SimpleRefCount<CybertwinAssetManager>
{
  public:
    CybertwinAssetManager();
    CybertwinAssetManager(CYBERTWINID_t);

  private:
    CYBERTWINID_t m_cuid;
    uint16_t m_credit;
    std::vector<CYBERTWINID_t> m_loginRecords;
    std::vector<std::string> m_loginCities;
    uint64_t m_avgBytesTransferredPerFlow;
    uint64_t m_maxBytesTransferredPerFlow;
    uint64_t m_avgPeerContactedPerSession;
    uint64_t m_maxPeerContactedPerSession;
    uint64_t m_aggregatedRecords;
    uint16_t m_recentViolationCount;
    Time m_lastVisitedTime;
};
class Cybertwin;

class CybertwinFirewall : public Application
{
  public:
    typedef Callback<uint16_t, CYBERTWINID_t> LookupCredit_cb;
    typedef Callback<void, CYBERTWINID_t, int> UpdateCredit_cb;

    static TypeId GetTypeId();
    CybertwinFirewall(CYBERTWINID_t cuid = 0,
                      LookupCredit_cb lookup = MakeNullCallback<uint16_t, CYBERTWINID_t>(),
                      UpdateCredit_cb update = MakeNullCallback<void, CYBERTWINID_t, int>());
    ~CybertwinFirewall();

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

    LookupCredit_cb LookupCredit;
    UpdateCredit_cb UpdateCredit;
    uint16_t GetCredit() const;

    FirewallState_t m_state;
    uint16_t m_ingressCredit;

    CYBERTWINID_t m_cuid;
    CYBERTWINID_t m_user;

    Time m_burstTs;
    uint64_t m_burstBytes;
    uint64_t m_burstLimit;

    Time m_flowTs;
    uint64_t m_flowBytes;
    uint64_t m_flowLimit;
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

    uint16_t LookupCredit(CYBERTWINID_t);
    void UpdateCredit(CYBERTWINID_t, int);

    void AssignInterfaces(CYBERTWIN_INTERFACE_LIST_t&);

    Address m_localAddr;
    uint64_t m_localPort;
    std::vector<Ipv4Address> m_localIpv4AddrList;

    Ptr<Socket> m_socket;
    std::unordered_map<CYBERTWINID_t, Ptr<Cybertwin>> m_cybertwinTable;
    std::unordered_map<CYBERTWINID_t, Ptr<CybertwinFirewall>> m_firewallTable;
    // std::unordered_map<CYBERTWINID_t, json> m_cybertwinAssetTable;

    std::unordered_set<uint16_t> m_assignedPorts;
    uint16_t m_lastAssignedPort;
};

} // namespace ns3

#endif