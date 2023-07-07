#ifndef CYBERTWIN_NODE_H
#define CYBERTWIN_NODE_H

#include "cybertwin-common.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

#include <vector>
#include <unordered_map>

namespace ns3
{

class CybertwinNode : public Node
{
  public:
    CybertwinNode();
    ~CybertwinNode();

    static TypeId GetTypeId();
    virtual TypeId GetInstanceTypeId() const;

    virtual void SetAddressList(std::vector<Ipv4Address> addressList);
    virtual void SetName(std::string name);

    virtual void PowerOn();
    
    void AddParent(Ptr<Node> parent);
    void AddConfigFile(std::string filename, nlohmann::json config);
    void InstallCNRSApp();

  protected:
    std::vector<Ipv4Address> ipv4AddressList;
    Ipv4Address m_upperNodeAddress; // ip address of default cybertwin controller
    Ipv4Address m_selfNodeAddress;  // ip address of the current node
    CYBERTWINID_t m_cybertwinId;
    std::string m_name;
    std::vector<Ptr<Node>> m_parents;
    std::unordered_map<std::string, nlohmann::json> m_configFiles;
};

} // namespace ns3

#endif