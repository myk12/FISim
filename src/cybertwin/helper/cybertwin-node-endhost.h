#ifndef CYBERTWIN_NODE_ENDHOST_H
#define CYBERTWIN_NODE_ENDHOST_H

#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/node.h"
#include "cybertwin-node.h"
#include "../model/cybertwin-client.h"

namespace ns3
{

class CybertwinEndHost : public CybertwinNode
{
  public:
    CybertwinEndHost();
    ~CybertwinEndHost();

    static TypeId GetTypeId();

    void Setup();

  private:
    Ptr<CybertwinConnClient> m_connClient;
    Ptr<CybertwinBulkClient> m_bulkClinet;
};

} // namespace ns3

#endif