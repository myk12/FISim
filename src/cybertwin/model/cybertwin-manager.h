#ifndef CYBERTWIN_MANAGER_H
#define CYBERTWIN_MANAGER_H

#include "cybertwin-common.h"
#include "cybertwin-header.h"
#include "cybertwin-multipath-controller.h"
#include "cybertwin-tag.h"
#include "cybertwin.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/callback.h"
#include "ns3/node.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns3
{
class Cybertwin;
class CybertwinManager : public Application
{
  public:
    static TypeId GetTypeId();
    CybertwinManager();
    ~CybertwinManager();

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

    void StartProxy();

    void AssignInterfaces(CYBERTWIN_INTERFACE_LIST_t&);

    void HandleCybertwinRegistration(Ptr<Socket>, Ptr<Packet>);
    void HandleCybertwinDestruction(Ptr<Socket>, Ptr<Packet>);
    void HandleCybertwinReconnect(Ptr<Socket>, Ptr<Packet>);

    Address m_localAddr;
    uint64_t m_localPort;
    std::vector<Ipv4Address> m_localIpv4AddrList;
    std::vector<Ipv4Address> m_globalIpv4AddrList;
    std::vector<Ptr<Ipv4Interface>> m_globalIpv4IfList;

    Ptr<Socket> m_proxySocket;
    uint16_t m_proxyPort;

    std::unordered_map<CYBERTWINID_t, Ptr<Cybertwin>> m_cybertwinTable;

    std::unordered_set<uint16_t> m_assignedPorts;
    uint16_t m_lastAssignedPort;
};

} // namespace ns3

#endif