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
                            .SetParent<CybertwinNode>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinEdgeServer>();

    return tid;
}

TypeId
CybertwinEdgeServer::GetInstanceTypeId() const
{
    return GetTypeId();
}

CybertwinEdgeServer::CybertwinEdgeServer()
    : m_cybertwinCNRSApp(nullptr),
      m_cybertwinControllerApp(nullptr)
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
    m_cybertwinCNRSApp = CreateObject<NameResolutionService>(m_upperNodeAddress);
    AddApplication(m_cybertwinCNRSApp);
    m_cybertwinCNRSApp->SetStartTime(Seconds(0.0));

    // install Cybertwin Controller application
    m_cybertwinControllerApp = CreateObject<CybertwinController>();
    m_cybertwinControllerApp->SetAttribute("LocalAddress", AddressValue(m_selfNodeAddress));
    AddApplication(m_cybertwinControllerApp);
    m_cybertwinControllerApp->SetStartTime(Seconds(0.0));
}

Ptr<NameResolutionService>
CybertwinEdgeServer::GetCNRSApp()
{
    return m_cybertwinCNRSApp;
}

Ptr<CybertwinController>
CybertwinEdgeServer::GetCtrlApp()
{
    return m_cybertwinControllerApp;
}

} // namespace ns3
