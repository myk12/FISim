#ifndef CYBERTWIN_NODE_ENDHOST_H
#define CYBERTWIN_NODE_ENDHOST_H

#include "cybertwin-client.h"
#include "cybertwin-node.h"

#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/node.h"

namespace ns3
{

class CybertwinEndHost : public CybertwinNode
{
  public:
    CybertwinEndHost();
    ~CybertwinEndHost();

    static TypeId GetTypeId();

    void PowerOn() override;
    void Connect(const CybertwinCertTag&);
    void SendTo(CYBERTWINID_t, uint32_t size = 0);

  private:
    Ptr<CybertwinConnClient> m_connClient;
    Ptr<CybertwinBulkClient> m_bulkClient;
};

} // namespace ns3

#endif