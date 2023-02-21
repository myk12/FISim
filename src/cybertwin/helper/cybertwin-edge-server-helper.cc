#include "cybertwin-edge-server-helper.h"

#include "ns3/names.h"

namespace ns3
{

CybertwinEdgeServerHelper::CybertwinEdgeServerHelper(Address address)
{
    m_factory.SetTypeId("ns3::CybertwinController");
    m_factory.Set("LocalAddress", AddressValue(address));
}

void
CybertwinEdgeServerHelper::SetAttribute(const std::string& name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

ApplicationContainer
CybertwinEdgeServerHelper::Install(Ptr<Node> node) const
{
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer
CybertwinEdgeServerHelper::Install(const std::string& nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer
CybertwinEdgeServerHelper::Install(NodeContainer c) const
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        apps.Add(InstallPriv(*i));
    }
    return apps;
}

Ptr<Application>
CybertwinEdgeServerHelper::InstallPriv(Ptr<Node> node) const
{
    Ptr<Application> app = m_factory.Create<Application>();
    node->AddApplication(app);
    return app;
}

} // namespace ns3