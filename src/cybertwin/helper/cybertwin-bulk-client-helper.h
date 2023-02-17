#ifndef CYBERTWIN_BULK_CLIENT_HELPER_H
#define CYBERTWIN_BULK_CLIENT_HELPER_H

#include "ns3/address.h"
#include "ns3/application-container.h"
#include "ns3/attribute.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

namespace ns3
{

class CybertwinBulkClientHelper
{
  public:
    CybertwinBulkClientHelper(uint64_t, uint64_t, Address, Address);

    void SetAttribute(std::string name, const AttributeValue& value);

    ApplicationContainer Install(NodeContainer c) const;
    ApplicationContainer Install(Ptr<Node> node) const;

  private:
    Ptr<Application> InstallPriv(Ptr<Node>) const;
    ObjectFactory m_factory;
};

} // namespace ns3

#endif