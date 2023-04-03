#include "cybertwin-cert.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinCert");
NS_OBJECT_ENSURE_REGISTERED(CybertwinCert);
NS_OBJECT_ENSURE_REGISTERED(CybertwinCertificate);

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
    return sizeof(m_cuid) + sizeof(m_credit) + sizeof(m_ingressCredit);
}

void
CybertwinCertificate::Serialize(TagBuffer i) const
{
    i.WriteU64(m_cuid);
    i.WriteU16(m_credit);
    i.WriteU16(m_ingressCredit);
}

void
CybertwinCertificate::Deserialize(TagBuffer i)
{
    m_cuid = i.ReadU64();
    m_credit = i.ReadU16();
    m_ingressCredit = i.ReadU16();
}

void
CybertwinCertificate::Print(std::ostream& os) const
{
    os << "cybertwin=" << m_cuid << ", credit=" << m_credit
       << ", ingressCredit=" << m_ingressCredit;
}

std::string
CybertwinCertificate::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

void
CybertwinCertificate::SetCybertwin(CYBERTWINID_t val)
{
    m_cuid = val;
}

CYBERTWINID_t
CybertwinCertificate::GetCybertwin() const
{
    return m_cuid;
}

void
CybertwinCertificate::SetCredit(uint16_t val)
{
    m_credit = val;
}

uint16_t
CybertwinCertificate::GetCredit() const
{
    return m_credit;
}

void
CybertwinCertificate::SetIngressCredit(uint16_t val)
{
    m_ingressCredit = val;
}

uint16_t
CybertwinCertificate::GetIngressCredit() const
{
    return m_ingressCredit;
}

CybertwinCert::CybertwinCert()
    : m_cybertwinId(0),
      m_credit(0),
      m_ingressCredit(0),
      m_isCreditFixed(false),
      m_isUserAuthRequired(false),
      m_isCertValid(false)
{
}

CybertwinCert::CybertwinCert(CYBERTWINID_t cid,
                             uint16_t initCredit,
                             uint16_t ingressCredit,
                             bool isCreditFixed,
                             bool isUserAuthRequired,
                             bool isCertValid)
    : m_cybertwinId(cid),
      m_credit(initCredit),
      m_ingressCredit(ingressCredit),
      m_isCreditFixed(isCreditFixed),
      m_isUserAuthRequired(isUserAuthRequired),
      m_isCertValid(isCertValid)
{
}

TypeId
CybertwinCert::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinCert")
                            .SetParent<Header>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinCert>();
    return tid;
}

TypeId
CybertwinCert::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CybertwinCert::Print(std::ostream& os) const
{
    os << "(CUID=" << m_cybertwinId << ", Credit=" << m_credit
       << ", IngressCredit=" << m_ingressCredit << ", IsCreditFixed=" << m_isCreditFixed
       << ", IsUserAuthRequired=" << m_isUserAuthRequired << ", IsCertValid=" << m_isCertValid
       << ")";
}

std::string
CybertwinCert::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

uint32_t
CybertwinCert::GetSerializedSize() const
{
    return sizeof(m_cybertwinId) + sizeof(m_credit) + sizeof(m_ingressCredit) +
           sizeof(m_isCreditFixed) + sizeof(m_isUserAuthRequired) + sizeof(m_isCertValid);
}

void
CybertwinCert::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU64(m_cybertwinId);
    i.WriteHtonU16(m_credit);
    i.WriteHtonU16(m_ingressCredit);
    i.WriteU8(m_isCreditFixed);
    i.WriteU8(m_isUserAuthRequired);
    i.WriteU8(m_isCertValid);
}

uint32_t
CybertwinCert::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_cybertwinId = i.ReadNtohU64();
    m_credit = i.ReadNtohU16();
    m_ingressCredit = i.ReadNtohU16();
    m_isCreditFixed = i.ReadU8();
    m_isUserAuthRequired = i.ReadU8();
    m_isCertValid = i.ReadU8();
    return GetSerializedSize();
}

void
CybertwinCert::SetCybertwinId(CYBERTWINID_t cybertwinId)
{
    m_cybertwinId = cybertwinId;
}

CYBERTWINID_t
CybertwinCert::GetCybertwinId() const
{
    return m_cybertwinId;
}

void
CybertwinCert::SetCredit(uint16_t initCredit)
{
    m_credit = initCredit;
}

uint16_t
CybertwinCert::GetCredit() const
{
    return m_credit;
}

void
CybertwinCert::SetIngressCredit(uint16_t ingressCredit)
{
    m_ingressCredit = ingressCredit;
}

uint16_t
CybertwinCert::GetIngressCredit() const
{
    return m_ingressCredit;
}

void
CybertwinCert::SetIsCreditFixed(bool isCreditFixed)
{
    m_isCreditFixed = isCreditFixed;
}

bool
CybertwinCert::GetIsCreditFixed() const
{
    return m_isCreditFixed;
}

void
CybertwinCert::SetIsUserAuthRequired(bool isUserAuthRequired)
{
    m_isUserAuthRequired = isUserAuthRequired;
}

bool
CybertwinCert::GetIsUserAuthRequired() const
{
    return m_isUserAuthRequired;
}

void
CybertwinCert::SetIsCertValid(bool isCertValid)
{
    m_isCertValid = isCertValid;
}

bool
CybertwinCert::GetIsCertValid() const
{
    return m_isCertValid;
}

} // namespace ns3