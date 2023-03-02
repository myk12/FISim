#include "ns3/cybertwin-common.h"
#include "ns3/cybertwin-name-resolution-service.h"
#include "cybertwin-node-coreserver.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinCoreServer");

NS_OBJECT_ENSURE_REGISTERED(CybertwinCoreServer);

TypeId
CybertwinCoreServer::GetTypeId()
{
    static TypeId tid = 
        TypeId("ns3::CybertwinCoreServer")
            .SetParent<Node>()
            .SetGroupName("Cybertwin")
            .AddConstructor<CybertwinCoreServer>();
    return tid;
}

CybertwinCoreServer::CybertwinCoreServer():
    cybertwinCNRSApp(nullptr)
{
    NS_LOG_DEBUG("[CybertwinCoreServer] create CybertwinCoreServer.");

}

int32_t
CybertwinCoreServer::Setup()
{
    NS_LOG_DEBUG("[CybertwinCoreServer] create CybertwinCoreServer.");
    // install CNRS application
    cybertwinCNRSApp = CreateObject<NameResolutionService>();
    this->AddApplication(cybertwinCNRSApp);
    cybertwinCNRSApp->SetStartTime(Simulator::Now());

    return 0;
}

CybertwinCoreServer::~CybertwinCoreServer()
{
    NS_LOG_DEBUG("[CybertwinCoreServer] destroy CybertwinCoreServer.");
}

}

