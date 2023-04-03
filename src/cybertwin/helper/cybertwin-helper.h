#ifndef CYBERTWIN_HELPER_H
#define CYBERTWIN_HELPER_H

#include "../model/cybertwin-cert.h"
#include "../model/cybertwin-client.h"

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
    CybertwinHelper();
    CybertwinHelper(std::string);

    void SetAttribute(std::string, const AttributeValue& value);

    ApplicationContainer Install(NodeContainer) const;
    ApplicationContainer Install(Ptr<Node>) const;

  protected:
    Ptr<Application> InstallPriv(Ptr<Node>) const;
    ObjectFactory m_factory;
};

class CybertwinConnHelper : public CybertwinHelper
{
  public:
    CybertwinConnHelper();

    void SetDevCert(Ptr<Node>, const CybertwinCert&);
    void SetUsrCert(Ptr<Node>, const CybertwinCert&);
};

} // namespace ns3

#endif /* CYBERTWIN_HELPER_H */
