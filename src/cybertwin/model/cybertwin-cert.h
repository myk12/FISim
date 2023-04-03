#ifndef CYBERTWIN_CERT_H
#define CYBERTWIN_CERT_H

#include "cybertwin-common.h"

#include "ns3/header.h"
#include "ns3/tag.h"

namespace ns3
{

class CybertwinCertificate : public Tag
{
  public:
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer) const override;
    void Deserialize(TagBuffer) override;
    void Print(std::ostream&) const override;
    std::string ToString() const;

    void SetCybertwin(CYBERTWINID_t);
    CYBERTWINID_t GetCybertwin() const;
    void SetCredit(uint16_t);
    uint16_t GetCredit() const;
    void SetIngressCredit(uint16_t);
    uint16_t GetIngressCredit() const;

  private:
    CYBERTWINID_t m_cuid;
    uint16_t m_credit;
    uint16_t m_ingressCredit;
};

/**
 * @brief For simplicity, we simulate the certificate file as a TCP packet header
 */
class CybertwinCert : public Header
{
  public:
    CybertwinCert();
    CybertwinCert(CYBERTWINID_t, uint16_t, uint16_t, bool, bool, bool);

    void SetCybertwinId(CYBERTWINID_t);
    CYBERTWINID_t GetCybertwinId() const;

    void SetCredit(uint16_t);
    uint16_t GetCredit() const;

    void SetIngressCredit(uint16_t);
    uint16_t GetIngressCredit() const;

    void SetIsCreditFixed(bool);
    bool GetIsCreditFixed() const;

    void SetIsUserAuthRequired(bool);
    bool GetIsUserAuthRequired() const;

    // simulates if the certificate is valid or not
    void SetIsCertValid(bool);
    bool GetIsCertValid() const;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream&) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator) const override;
    uint32_t Deserialize(Buffer::Iterator) override;
    std::string ToString() const;

  private:
    CYBERTWINID_t m_cybertwinId;
    uint16_t m_credit;
    uint16_t m_ingressCredit;

    bool m_isCreditFixed;
    bool m_isUserAuthRequired;
    bool m_isCertValid;
};

} // namespace ns3

#endif