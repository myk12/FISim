#include "cybertwin-core-server.h"

#include "cybertwin-common.h"
#include "cybertwin-name-resolution-service.h"

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
CybertwinCoreServer::PowerOn()
{
    if (m_parents.size() != 0)
    {
        // has parents, install CNRS application
        // only root node has no parents
        InstallCNRSApp();
    }
}

CybertwinCoreServer::~CybertwinCoreServer()
{
    NS_LOG_DEBUG("[CybertwinCoreServer] destroy CybertwinCoreServer.");
}

} // namespace ns3
