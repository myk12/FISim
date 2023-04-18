#ifndef CYBERTWIN_NODE_ENDHOST_H
#define CYBERTWIN_NODE_ENDHOST_H

#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/node.h"
#include "../model/cybertwin-client.h"

namespace ns3
{

class CybertwinEndHost : public Node
{
  public:
    CybertwinEndHost();
    ~CybertwinEndHost();

    static TypeId GetTypeId();

    int32_t Setup(Ipv4Address edgeServerAddr);

  private:
    Ipv4Address edgeServerAddress; // ip address of default cybertwin controller
    Ptr<CybertwinConnClient> m_connClient;
    Ptr<CybertwinBulkClient> m_bulkClinet;
};

} // namespace ns3

#endif