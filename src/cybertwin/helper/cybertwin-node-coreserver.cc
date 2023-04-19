#include "cybertwin-node-coreserver.h"

#include "../model/cybertwin-common.h"
#include "../model/cybertwin-name-resolution-service.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinCoreServer");

NS_OBJECT_ENSURE_REGISTERED(CybertwinCoreServer);

TypeId
CybertwinCoreServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinCoreServer")
                            .SetParent<Node>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinCoreServer>();
    return tid;
}

CybertwinCoreServer::CybertwinCoreServer()
    : cybertwinCNRSApp(nullptr)
{
    NS_LOG_DEBUG("Creating a CybertwinCoreServer.");
}

void
CybertwinCoreServer::Setup()
{
    NS_LOG_DEBUG("[CybertwinCoreServer] create CybertwinCoreServer.");
    // install CNRS application
    cybertwinCNRSApp = CreateObject<NameResolutionService>(upperNodeAddress);
    this->AddApplication(cybertwinCNRSApp);
    cybertwinCNRSApp->SetStartTime(Simulator::Now());
}

CybertwinCoreServer::~CybertwinCoreServer()
{
    NS_LOG_DEBUG("[CybertwinCoreServer] destroy CybertwinCoreServer.");
}

} // namespace ns3
