#ifndef CYBERTWIN_PACKET_HEADER_H
#define CYBERTWIN_PACKET_HEADER_H

#include "ns3/header.h"

namespace ns3
{

class CybertwinPacketHeader : public Header
{
  public:
    CybertwinPacketHeader(uint64_t src = 0, uint64_t dst = 0, uint32_t size = 0, uint16_t cmd = 0);

    void SetSrc(uint64_t src);
    uint64_t GetSrc() const;

    void SetDst(uint64_t dst);
    uint64_t GetDst() const;

    void SetSize(uint32_t size);
    uint32_t GetSize() const;

    void SetCmd(uint16_t cmd);
    uint16_t GetCmd() const;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    std::string ToString() const;

  private:
    uint64_t m_src;
    uint64_t m_dst;
    uint32_t m_size;
    uint16_t m_cmd;
};

} // namespace ns3

#endif