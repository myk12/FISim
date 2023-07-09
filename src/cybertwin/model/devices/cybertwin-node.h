#ifndef CYBERTWIN_NODE_H
#define CYBERTWIN_NODE_H

#include "../cybertwin-common.h"
#include "../apps/cybertwin-manager.h"
#include "../networks/cybertwin-name-resolution-service.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/node.h"

#include <vector>
#include <unordered_map>

namespace ns3
{
class CybertwinManager;
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
    void InstallCybertwinManagerApp();

    Ptr<NameResolutionService> GetCNRSApp();

  protected:
    std::vector<Ipv4Address> ipv4AddressList;
    Ipv4Address m_upperNodeAddress; // ip address of default cybertwin controller
    Ipv4Address m_selfNodeAddress;  // ip address of the current node
    CYBERTWINID_t m_cybertwinId;

    Ptr<NameResolutionService> m_cybertwinCNRSApp;
    std::string m_name;
    std::vector<Ptr<Node>> m_parents;
    std::unordered_map<std::string, nlohmann::json> m_configFiles;
};


//**********************************************************************
//*               edge server node                                     *
//**********************************************************************
class CybertwinEdgeServer: public CybertwinNode
{
  public:
    CybertwinEdgeServer();
    ~CybertwinEdgeServer();

    static TypeId GetTypeId();

    void PowerOn();

    Ptr<CybertwinManager> GetCtrlApp();

  private:
    Ptr<NameResolutionService> m_cybertwinCNRSApp;
    Ptr<CybertwinManager> m_CybertwinManagerApp;
};

//**********************************************************************
//*               core server node                                     *
//**********************************************************************
class CybertwinCoreServer: public CybertwinNode
{
    public:
        CybertwinCoreServer();
        ~CybertwinCoreServer();

        static TypeId GetTypeId();
        
        void PowerOn() override;
    
    private:
        Ipv4Address CNRSUpNodeAddress;
        Ptr<Application> cybertwinCNRSApp;
};

//**********************************************************************
//*               end host node                                        *
//**********************************************************************

class CybertwinEndHost : public CybertwinNode
{
  public:
    CybertwinEndHost();
    ~CybertwinEndHost();

    static TypeId GetTypeId();

    void PowerOn() override;
    //void Connect(const CybertwinCertTag&);
    //void SendTo(CYBERTWINID_t, uint32_t size = 0);

  private:
    // private member functions
    //Ptr<CybertwinConnClient> m_connClient;
    //Ptr<CybertwinBulkClient> m_bulkClient;
  private:
    // private member variables
    Ptr<Socket> m_cybertwinSocket;
};

} // namespace ns3

#endif