#ifndef CYBERTWIN_TAG_H
#define CYBERTWIN_TAG_H

#include "cybertwin-common.h"

#include "ns3/header.h"
#include "ns3/tag.h"

namespace ns3
{

class CybertwinTag : public Tag
{
  public:
    CybertwinTag(CYBERTWINID_t cuid = 0);
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer) const override;
    void Deserialize(TagBuffer) override;
    void Print(std::ostream&) const override;
    std::string ToString() const;

    void SetCybertwin(CYBERTWINID_t);
    CYBERTWINID_t GetCybertwin() const;

  protected:
    CYBERTWINID_t m_cuid;
};

class CybertwinCreditTag : public CybertwinTag
{
  public:
    CybertwinCreditTag(uint16_t credit = 0, CYBERTWINID_t cuid = 0, CYBERTWINID_t peer = 0);
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer) const override;
    void Deserialize(TagBuffer) override;
    void Print(std::ostream&) const override;

    void SetCredit(uint16_t);
    uint16_t GetCredit() const;
    void SetPeer(CYBERTWINID_t);
    CYBERTWINID_t GetPeer() const;

  private:
    uint16_t m_credit;
    CYBERTWINID_t m_peer;
};

class CybertwinCertTag : public CybertwinTag
{
  public:
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer) const override;
    void Deserialize(TagBuffer) override;
    void Print(std::ostream&) const override;

    void SetInitialCredit(uint16_t);
    uint16_t GetInitialCredit() const;
    void SetIngressCredit(uint16_t);
    uint16_t GetIngressCredit() const;
    void SetIsUserRequired(bool);
    bool GetIsUserRequired() const;
    void SetIsValid(bool);
    bool GetIsValid() const;
    void AddUser(CYBERTWINID_t, uint16_t, uint16_t);
    CYBERTWINID_t GetUser() const;
    uint16_t GetUserInitialCredit() const;

  private:
    uint16_t m_initialCredit;
    uint16_t m_ingressCredit;
    bool m_isUserRequired;
    bool m_isCertValid;

    CYBERTWINID_t m_usr;
    uint16_t m_usrInitialCredit;
};

} // namespace ns3

#endif