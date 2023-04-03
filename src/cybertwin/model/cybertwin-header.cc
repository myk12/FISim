#include "cybertwin-header.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinHeader");
NS_OBJECT_ENSURE_REGISTERED(CybertwinHeader);

TypeId
CybertwinHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinHeader")
                            .SetParent<Header>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinHeader>();
    return tid;
}

TypeId
CybertwinHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

CybertwinHeader::CybertwinHeader()
    : m_command(0),
      m_cybertwin(0),
      m_peer(0),
      m_size(0)
{
    m_credit.score = 0;
}

void
CybertwinHeader::Print(std::ostream& os) const
{
    os << "(command=" << int(m_command) << ", cybertwin=" << m_cybertwin << ", peer=" << m_peer
       << ", size=" << m_size;
    if (m_command == HOST_CONNECT)
    {
        os << ", isLatestOs=" << bool(m_credit.meta.isLatestOs)
           << ", isLatestPatch=" << bool(m_credit.meta.isLatestPatch) << ")";
    }
    else
    {
        os << ", credit=" << m_credit.score << ")";
    }
}

std::string
CybertwinHeader::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

uint32_t
CybertwinHeader::GetSerializedSize() const
{
    return sizeof(m_command) + sizeof(m_cybertwin) + sizeof(m_credit) +
           (isDataPacket() ? sizeof(m_peer) + sizeof(m_size) : 0);
}

void
CybertwinHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_command);
    i.WriteHtonU64(m_cybertwin);
    i.WriteHtonU16(m_credit.score);
    if (isDataPacket())
    {
        i.WriteHtonU64(m_peer);
        i.WriteHtonU32(m_size);
    }
}

uint32_t
CybertwinHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_command = i.ReadU8();
    m_cybertwin = i.ReadNtohU64();
    m_credit.score = i.ReadNtohU16();
    if (isDataPacket())
    {
        m_peer = i.ReadNtohU64();
        m_size = i.ReadNtohU32();
    }
    return GetSerializedSize();
}

bool
CybertwinHeader::isDataPacket() const
{
    return m_command == HOST_SEND || m_command == CYBERTWIN_SEND;
}

void
CybertwinHeader::SetCommand(uint8_t val)
{
    m_command = val;
}

uint8_t
CybertwinHeader::GetCommand() const
{
    return m_command;
}

void
CybertwinHeader::SetCybertwin(CYBERTWINID_t val)
{
    m_cybertwin = val;
}

CYBERTWINID_t
CybertwinHeader::GetCybertwin() const
{
    return m_cybertwin;
}

void
CybertwinHeader::SetCredit(uint16_t val)
{
    m_credit.score = val;
}

uint16_t
CybertwinHeader::GetCredit() const
{
    return m_credit.score;
}

void
CybertwinHeader::SetPeer(CYBERTWINID_t val)
{
    m_peer = val;
}

CYBERTWINID_t
CybertwinHeader::GetPeer() const
{
    return m_peer;
}

void
CybertwinHeader::SetIsLatestOs(bool val)
{
    m_credit.meta.isLatestOs = val;
}

bool
CybertwinHeader::GetIsLatestOs() const
{
    return m_credit.meta.isLatestOs;
}

void
CybertwinHeader::SetIsLatestPatch(bool val)
{
    m_credit.meta.isLatestPatch = val;
}

bool
CybertwinHeader::GetIsLatestPatch() const
{
    return m_credit.meta.isLatestPatch;
}

void
CybertwinHeader::SetSize(uint32_t val)
{
    m_size = val;
}

uint32_t
CybertwinHeader::GetSize() const
{
    return m_size;
}

} // namespace ns3