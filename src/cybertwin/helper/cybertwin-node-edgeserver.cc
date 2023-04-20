#include "cybertwin-node-edgeserver.h"

#include "../model/cybertwin-common.h"
#include "../model/cybertwin-edge.h"
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
}

CybertwinEdgeServer::~CybertwinEdgeServer()
{
}

void
CybertwinEdgeServer::Setup()
{
    NS_LOG_FUNCTION(GetId());
    // install CNRS application
    cybertwinCNRSApp = CreateObject<NameResolutionService>(m_upperNodeAddress);
    AddApplication(cybertwinCNRSApp);
    cybertwinCNRSApp->SetStartTime(Seconds(0));

    // install Cybertwin Controller application

    cybertwinControllerApp =
        CreateObject<CybertwinController>(MakeCallback(&CybertwinEdgeServer::UpdateCNRS, this));
    cybertwinControllerApp->SetAttribute("LocalAddress", AddressValue(m_selfNodeAddress));
    AddApplication(cybertwinControllerApp);
    cybertwinControllerApp->SetStartTime(Seconds(0));
}

bool
CybertwinEdgeServer::UpdateCNRS(CYBERTWINID_t cuid, CYBERTWIN_INTERFACE_LIST_t& interface)
{
    cybertwinCNRSApp->InsertCybertwinInterfaceName(cuid, interface);
    // TODO: whether inserted successfully
    return true;
}

} // namespace ns3
