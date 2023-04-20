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

    void Setup() override;
    bool UpdateCNRS(CYBERTWINID_t, CYBERTWIN_INTERFACE_LIST_t&);

  private:
    Ipv4Address CNRSUpNodeAddress;
    Ptr<NameResolutionService> cybertwinCNRSApp;
    Ptr<CybertwinController> cybertwinControllerApp;
};
} // namespace ns3

#endif