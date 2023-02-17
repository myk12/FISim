#include "cybertwin-bulk-client-helper.h"

#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

namespace ns3
{

CybertwinBulkClientHelper::CybertwinBulkClientHelper(uint64_t localGuid,
                                                     uint64_t peerGuid,
                                                     Address localAddr,
                                                     Address edgeAddr)
{
    m_factory.SetTypeId("ns3::CybertwinBulkClient");
    m_factory.Set("LocalGuid", UintegerValue(localGuid));
    m_factory.Set("PeerGuid", UintegerValue(peerGuid));
    m_factory.Set("LocalAddress", AddressValue(localAddr));
    m_factory.Set("EdgeAddress", AddressValue(edgeAddr));
}

void
CybertwinBulkClientHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

ApplicationContainer
CybertwinBulkClientHelper::Install(Ptr<Node> node) const
{
    return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer
CybertwinBulkClientHelper::Install(NodeContainer c) const
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        apps.Add(InstallPriv(*i));
    }

    return apps;
}

Ptr<Application>
CybertwinBulkClientHelper::InstallPriv(Ptr<Node> node) const
{
    Ptr<Application> app = m_factory.Create<Application>();
    node->AddApplication(app);

    return app;
}

} // namespace ns3