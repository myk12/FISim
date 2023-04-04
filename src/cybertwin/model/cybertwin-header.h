#ifndef CYBERTWIN_HEADER_H
#define CYBERTWIN_HEADER_H

#include "cybertwin-common.h"

#include "ns3/header.h"

namespace ns3
{

enum CybertwinCommand_t
{
    // Control commands
    HOST_CONNECT = 96,
    HOST_DISCONNECT,
    CYBERTWIN_CONNECT_SUCCESS,
    CYBERTWIN_CONNECT_ERROR,
    // Data commands
    DATA,
};

class CybertwinHeader : public Header
{
  public:
    CybertwinHeader();
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream&) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator) const override;
    uint32_t Deserialize(Buffer::Iterator) override;
    std::string ToString() const;

    bool isDataPacket() const;

    void SetCommand(uint8_t);
    uint8_t GetCommand() const;

    void SetCybertwin(CYBERTWINID_t);
    CYBERTWINID_t GetCybertwin() const;

    void SetPeer(CYBERTWINID_t);
    CYBERTWINID_t GetPeer() const;

    void SetSize(uint32_t);
    uint32_t GetSize() const;

    void SetCybertwinPort(uint16_t);
    uint16_t GetCybertwinPort() const;

  protected:
    uint8_t m_command;
    CYBERTWINID_t m_cybertwin;

    // data packet payload
    CYBERTWINID_t m_peer;
    uint32_t m_size;

    // control packet payload
    uint16_t m_cybertwinPort;
};

} // namespace ns3

#endif