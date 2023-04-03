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
    HOST_SEND,
    CYBERTWIN_SEND
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

    void SetCredit(uint16_t);
    uint16_t GetCredit() const;

    void SetIsLatestOs(bool);
    bool GetIsLatestOs() const;

    void SetIsLatestPatch(bool);
    bool GetIsLatestPatch() const;

    void SetSize(uint32_t);
    uint32_t GetSize() const;

  protected:
    uint8_t m_command;
    CYBERTWINID_t m_cybertwin;

    union {
        struct
        {
            bool isLatestOs;
            bool isLatestPatch;
        } meta;

        uint16_t score;
    } m_credit;

    CYBERTWINID_t m_peer;
    uint32_t m_size;
};

} // namespace ns3

#endif