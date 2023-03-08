#include "cybertwin-node-edgeserver.h"

#include "../model/cybertwin-common.h"
#include "../model/cybertwin-edge-server.h"
#include "../model/cybertwin-name-resolution-service.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinEdgeServer");

NS_OBJECT_ENSURE_REGISTERED(CybertwinEdgeServer);

TypeId
CybertwinEdgeServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinEdgeServer")
                            .SetParent<Node>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinEdgeServer>();

    return tid;
}

CybertwinEdgeServer::CybertwinEdgeServer()
    : cybertwinCNRSApp(nullptr),
      cybertwinControllerApp(nullptr)
{
    NS_LOG_DEBUG("[CybertwinEdgeServer] create CybertwinEdgeServer.");
}

CybertwinEdgeServer::~CybertwinEdgeServer()
{
    NS_LOG_DEBUG("[CybertwinEdgeServer] destroy CybertwinEdgeServer.");
}

int32_t
CybertwinEdgeServer::Setup(Ipv4Address upNodeAddress)
{
    NS_LOG_DEBUG("[CybertwinEdgeServer] create CybertwinEdgeServer.");

    CNRSUpNodeAddress = upNodeAddress;

    // install CNRS application
    cybertwinCNRSApp = CreateObject<NameResolutionService>(CNRSUpNodeAddress);
    this->AddApplication(cybertwinCNRSApp);
    cybertwinCNRSApp->SetStartTime(Simulator::Now());

    // install Cybertwin Controller application
    cybertwinControllerApp = CreateObject<CybertwinController>();
    this->AddApplication(cybertwinControllerApp);
    cybertwinControllerApp->SetStartTime(Simulator::Now());

    return 0;
}

} // namespace ns3
