#include "cybertwin-node-endhost.h"

#include "../model/cybertwin-common.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinEndHost");

NS_OBJECT_ENSURE_REGISTERED(CybertwinEndHost);

TypeId
CybertwinEndHost::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinEndHost")
                            .SetParent<Node>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinEndHost>();

    return tid;
}

CybertwinEndHost::CybertwinEndHost()
    :m_connClient(nullptr),
    m_bulkClinet(nullptr)
{
    NS_LOG_DEBUG("[CybertwinEndHost] create CybertwinEndHost.");
}

CybertwinEndHost::~CybertwinEndHost()
{
    NS_LOG_DEBUG("[CybertwinEndHost] destroy CybertwinEndHost.");
}

void
CybertwinEndHost::Setup()
{
    NS_LOG_DEBUG("Setting up a CybertwinEndHost.");

    // install endhost app
    //m_connClient = CreateObject<CybertwinConnClient>();
    //this->AddApplication(m_connClient);
    //m_connClient->SetStartTime(Simulator::Now());

    //m_bulkClinet = CreateObject<CybertwinBulkClient>();
    //this->AddApplication(m_bulkClinet);
    //m_bulkClinet->SetStartTime(Simulator::Now());
}

} // namespace ns3