#include "cybertwin-tag.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinCert");
NS_OBJECT_ENSURE_REGISTERED(CybertwinTag);
NS_OBJECT_ENSURE_REGISTERED(CybertwinCreditTag);
NS_OBJECT_ENSURE_REGISTERED(CybertwinCertTag);

CybertwinTag::CybertwinTag(CYBERTWINID_t cuid)
    : m_cuid(cuid)
{
}

TypeId
CybertwinTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinTag")
                            .SetParent<Tag>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinTag>();
    return tid;
}

TypeId
CybertwinTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
CybertwinTag::GetSerializedSize() const
{
    return sizeof(m_cuid);
}

void
CybertwinTag::Serialize(TagBuffer i) const
{
    i.WriteU64(m_cuid);
}

void
CybertwinTag::Deserialize(TagBuffer i)
{
    m_cuid = i.ReadU64();
}

void
CybertwinTag::Print(std::ostream& os) const
{
    os << "cybertwin=" << m_cuid;
}

std::string
CybertwinTag::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

void
CybertwinTag::SetCybertwin(CYBERTWINID_t val)
{
    m_cuid = val;
}

CYBERTWINID_t
CybertwinTag::GetCybertwin() const
{
    return m_cuid;
}

CybertwinCreditTag::CybertwinCreditTag(uint16_t credit, CYBERTWINID_t cuid, CYBERTWINID_t peer)
    : m_credit(credit),
      m_peer(peer)
{
    m_cuid = cuid;
}

TypeId
CybertwinCreditTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinCreditTag")
                            .SetParent<CybertwinTag>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinCreditTag>();
    return tid;
}

TypeId
CybertwinCreditTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
CybertwinCreditTag::GetSerializedSize() const
{
    return sizeof(m_cuid) + sizeof(m_credit) + sizeof(m_peer);
}

void
CybertwinCreditTag::Serialize(TagBuffer i) const
{
    i.WriteU64(m_cuid);
    i.WriteU16(m_credit);
    i.WriteU64(m_peer);
}

void
CybertwinCreditTag::Deserialize(TagBuffer i)
{
    m_cuid = i.ReadU64();
    m_credit = i.ReadU16();
    m_peer = i.ReadU64();
}

void
CybertwinCreditTag::Print(std::ostream& os) const
{
    os << "cybertwin=" << m_cuid << ", credit=" << m_credit << ", peer=" << m_peer;
}

void
CybertwinCreditTag::SetCredit(uint16_t val)
{
    m_credit = val;
}

uint16_t
CybertwinCreditTag::GetCredit() const
{
    return m_credit;
}

void
CybertwinCreditTag::SetPeer(CYBERTWINID_t val)
{
    m_peer = val;
}

CYBERTWINID_t
CybertwinCreditTag::GetPeer() const
{
    return m_peer;
}

TypeId
CybertwinCertTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinCertTag")
                            .SetParent<Tag>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinCertTag>();
    return tid;
}

TypeId
CybertwinCertTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
CybertwinCertTag::GetSerializedSize() const
{
    return sizeof(m_cuid) + sizeof(m_initialCredit) + sizeof(m_ingressCredit) +
           sizeof(m_isUserRequired) + sizeof(m_isCertValid) +
           (m_isUserRequired ? sizeof(m_usr) + sizeof(m_usrInitialCredit) : 0);
}

void
CybertwinCertTag::Serialize(TagBuffer i) const
{
    i.WriteU64(m_cuid);
    i.WriteU16(m_initialCredit);
    i.WriteU16(m_ingressCredit);
    i.WriteU8(m_isUserRequired);
    i.WriteU8(m_isCertValid);
    if (m_isUserRequired)
    {
        i.WriteU64(m_usr);
        i.WriteU16(m_usrInitialCredit);
    }
}

void
CybertwinCertTag::Deserialize(TagBuffer i)
{
    m_cuid = i.ReadU64();
    m_initialCredit = i.ReadU16();
    m_ingressCredit = i.ReadU16();
    m_isUserRequired = i.ReadU8();
    m_isCertValid = i.ReadU8();
    if (m_isUserRequired)
    {
        m_usr = i.ReadU64();
        m_usrInitialCredit = i.ReadU16();
    }
}

void
CybertwinCertTag::Print(std::ostream& os) const
{
    os << "cybertwin=" << m_cuid << ", credit=" << m_initialCredit
       << ", ingressCredit=" << m_ingressCredit << ", isUserRequired=" << m_isUserRequired
       << ", isCertValid=" << m_isCertValid;
    if (m_isUserRequired)
    {
        os << ", user=" << m_usr << ", userInitialCredit=" << m_usrInitialCredit;
    }
}

void
CybertwinCertTag::SetInitialCredit(uint16_t val)
{
    m_initialCredit = val;
}

uint16_t
CybertwinCertTag::GetInitialCredit() const
{
    return m_initialCredit;
}

void
CybertwinCertTag::SetIngressCredit(uint16_t val)
{
    m_ingressCredit = std::max(m_ingressCredit, val);
}

uint16_t
CybertwinCertTag::GetIngressCredit() const
{
    return m_ingressCredit;
}

void
CybertwinCertTag::SetIsUserRequired(bool val)
{
    m_isUserRequired = val;
}

bool
CybertwinCertTag::GetIsUserRequired() const
{
    return m_isUserRequired;
}

void
CybertwinCertTag::SetIsValid(bool val)
{
    m_isCertValid = val;
}

bool
CybertwinCertTag::GetIsValid() const
{
    return m_isCertValid;
}

void
CybertwinCertTag::AddUser(CYBERTWINID_t user, uint16_t userCredit, uint16_t userIngressCredit)
{
    m_usr = user;
    m_usrInitialCredit = userCredit;
    m_ingressCredit = std::max(m_ingressCredit, userIngressCredit);
}

CYBERTWINID_t
CybertwinCertTag::GetUser() const
{
    return m_usr;
}

uint16_t
CybertwinCertTag::GetUserInitialCredit() const
{
    return m_usrInitialCredit;
}

} // namespace ns3