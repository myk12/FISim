#ifndef CYBERTWIN_NODE_EDGESERVER_H
#define CYBERTWIN_NODE_EDGESERVER_H

#include "../model/cybertwin-edge.h"
#include "../model/cybertwin-name-resolution-service.h"
#include "cybertwin-node.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

namespace ns3
{

class CybertwinController;

class CybertwinEdgeServer : public CybertwinNode
{
  public:
    CybertwinEdgeServer();
    ~CybertwinEdgeServer();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Setup() override;
    Ptr<NameResolutionService> GetCNRSApp();
    Ptr<CybertwinController> GetCtrlApp();

  private:
    Ptr<NameResolutionService> m_cybertwinCNRSApp;
    Ptr<CybertwinController> m_cybertwinControllerApp;
};

} // namespace ns3

#endif