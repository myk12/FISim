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
                            .SetParent<CybertwinNode>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinEndHost>();
    return tid;
}

CybertwinEndHost::CybertwinEndHost()
    : m_connClient(nullptr),
      m_bulkClient(nullptr)
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
    NS_LOG_FUNCTION(GetId());
    m_connClient = CreateObject<CybertwinConnClient>();
    m_bulkClient = CreateObject<CybertwinBulkClient>();
}

void
CybertwinEndHost::Connect(const CybertwinCertTag& cert)
{
    NS_LOG_FUNCTION(GetId());
    m_connClient->SetAttribute("LocalAddress", AddressValue(m_selfNodeAddress));
    m_connClient->SetAttribute("EdgeAddress", AddressValue(m_upperNodeAddress));
    m_connClient->SetAttribute("LocalCuid", UintegerValue(m_cybertwinId));
    m_connClient->SetCertificate(cert);
    AddApplication(m_connClient);
    m_connClient->SetStartTime(Seconds(0));
}

void
CybertwinEndHost::SendTo(CYBERTWINID_t peer, uint32_t size)
{
    NS_LOG_FUNCTION(GetId() << peer << size);
    // TODO: will this still work when peer changes?
    m_bulkClient->SetAttribute("PeerCuid", UintegerValue(peer));
    m_bulkClient->SetAttribute("MaxBytes", UintegerValue(size));
    AddApplication(m_bulkClient);
    m_bulkClient->SetStartTime(Seconds(0));
    m_bulkClient->SetStopTime(Seconds(NORMAL_SIM_SECONDS));
}

} // namespace ns3