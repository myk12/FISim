#include "cybertwin-node-endhost.h"
#include "ns3/cybertwin-bulk-client.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinEndHost");

NS_OBJECT_ENSURE_REGISTERED(CybertwinEndHost);

TypeId
CybertwinEndHost::GetTypeId()
{
    static TypeId tid = 
        TypeId("ns3::CybertwinEndHost")
            .SetParent<Node>()
            .SetGroupName("Cybertwin")
            .AddConstructor<CybertwinEndHost>();
    
    return tid;
}

CybertwinEndHost::CybertwinEndHost():
    cybertwinControllerClient(nullptr),
    cybertwinClient(nullptr)
{
    NS_LOG_DEBUG("[CybertwinEndHost] create CybertwinEndHost.");
}

CybertwinEndHost::~CybertwinEndHost()
{
    NS_LOG_DEBUG("[CybertwinEndHost] destroy CybertwinEndHost.");
}

int32_t
CybertwinEndHost::Setup(Ipv4Address edgeServerAddr)
{
    NS_LOG_DEBUG("[CybertwinEndHost] Setup.");
    edgeServerAddress = edgeServerAddr;

    // install endhost app
    cybertwinControllerClient = CreateObject<CybertwinBulkClient>();
    this->AddApplication(cybertwinControllerClient);
    cybertwinControllerClient->SetStartTime(Simulator::Now());

    return 0;
}

}