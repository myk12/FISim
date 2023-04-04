#include "cybertwin-tag.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinCert");
NS_OBJECT_ENSURE_REGISTERED(CybertwinTag);
NS_OBJECT_ENSURE_REGISTERED(CybertwinCreditTag);
NS_OBJECT_ENSURE_REGISTERED(CybertwinCertificate);

CybertwinTag::CybertwinTag(CYBERTWINID_t cuid)
    : m_cuid(cuid)
{
}

TypeId
CybertwinTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinTag")
                            .SetParent<Header>()
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

CybertwinCreditTag::CybertwinCreditTag(uint16_t credit)
    : m_credit(credit)
{
}

TypeId
CybertwinCreditTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinCreditTag")
                            .SetParent<Header>()
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
    return sizeof(m_credit);
}

void
CybertwinCreditTag::Serialize(TagBuffer i) const
{
    i.WriteU16(m_credit);
}

void
CybertwinCreditTag::Deserialize(TagBuffer i)
{
    m_credit = i.ReadU16();
}

void
CybertwinCreditTag::Print(std::ostream& os) const
{
    os << "credit=" << m_credit;
}

std::string
CybertwinCreditTag::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
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

TypeId
CybertwinCertificate::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinCertificate")
                            .SetParent<Header>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinCertificate>();
    return tid;
}

TypeId
CybertwinCertificate::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
CybertwinCertificate::GetSerializedSize() const
{
    return sizeof(m_initialCredit) + sizeof(m_ingressCredit) + sizeof(m_isUserRequired) +
           sizeof(m_isCertValid) +
           (m_isUserRequired ? sizeof(m_usr) + sizeof(m_usrInitialCredit) : 0);
}

void
CybertwinCertificate::Serialize(TagBuffer i) const
{
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
CybertwinCertificate::Deserialize(TagBuffer i)
{
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
CybertwinCertificate::Print(std::ostream& os) const
{
    os << "credit=" << m_initialCredit << ", ingressCredit=" << m_ingressCredit
       << ", isUserRequired=" << m_isUserRequired << ", isCertValid=" << m_isCertValid;
    if (m_isUserRequired)
    {
        os << ", user=" << m_usr << ", userInitialCredit=" << m_usrInitialCredit;
    }
}

std::string
CybertwinCertificate::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

void
CybertwinCertificate::SetInitialCredit(uint16_t val)
{
    m_initialCredit = val;
}

uint16_t
CybertwinCertificate::GetInitialCredit() const
{
    return m_initialCredit;
}

void
CybertwinCertificate::SetIngressCredit(uint16_t val)
{
    m_ingressCredit = std::max(m_ingressCredit, val);
}

uint16_t
CybertwinCertificate::GetIngressCredit() const
{
    return m_ingressCredit;
}

void
CybertwinCertificate::SetIsUserRequired(bool val)
{
    m_isUserRequired = val;
}

bool
CybertwinCertificate::GetIsUserRequired() const
{
    return m_isUserRequired;
}

void
CybertwinCertificate::SetIsValid(bool val)
{
    m_isCertValid = val;
}

bool
CybertwinCertificate::GetIsValid() const
{
    return m_isCertValid;
}

void
CybertwinCertificate::AddUser(CYBERTWINID_t user, uint16_t userCredit, uint16_t userIngressCredit)
{
    m_usr = user;
    m_usrInitialCredit = userCredit;
    m_ingressCredit = std::max(m_ingressCredit, userIngressCredit);
}

CYBERTWINID_t
CybertwinCertificate::GetUser() const
{
    return m_usr;
}

uint16_t
CybertwinCertificate::GetUserInitialCredit() const
{
    return m_usrInitialCredit;
}

} // namespace ns3