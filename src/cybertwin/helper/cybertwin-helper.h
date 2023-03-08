#ifndef CYBERTWIN_HELPER_H
#define CYBERTWIN_HELPER_H

#include "ns3/application-container.h"
#include "ns3/attribute.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/string.h"

namespace ns3
{

class CybertwinHelper
{
  public:
    CybertwinHelper(std::string);

    void SetAttribute(std::string, const AttributeValue& value);

    ApplicationContainer Install(NodeContainer) const;
    ApplicationContainer Install(Ptr<Node>) const;

  private:
    Ptr<Application> InstallPriv(Ptr<Node>) const;
    ObjectFactory m_factory;
};

} // namespace ns3

#endif /* CYBERTWIN_HELPER_H */
