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
      m_size(0),
      m_cybertwinPort(0)
{
}

void
CybertwinHeader::Print(std::ostream& os) const
{
    os << "command=" << int(m_command) << ", cybertwin=" << m_cybertwin;
    if (isDataPacket())
    {
        os << ", peer=" << m_peer << ", size=" << m_size;
    }
    else
    {
        os << ", port=" << m_cybertwinPort;
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
    return sizeof(m_command) + sizeof(m_cybertwin) +
           (isDataPacket() ? sizeof(m_peer) + sizeof(m_size) : sizeof(m_cybertwinPort));
}

void
CybertwinHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_command);
    i.WriteHtonU64(m_cybertwin);
    if (isDataPacket())
    {
        i.WriteHtonU64(m_peer);
        i.WriteHtonU32(m_size);
    }
    else
    {
        i.WriteHtonU16(m_cybertwinPort);
    }
}

uint32_t
CybertwinHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_command = i.ReadU8();
    m_cybertwin = i.ReadNtohU64();
    if (isDataPacket())
    {
        m_peer = i.ReadNtohU64();
        m_size = i.ReadNtohU32();
    }
    else
    {
        m_cybertwinPort = i.ReadNtohU16();
    }
    return GetSerializedSize();
}

bool
CybertwinHeader::isDataPacket() const
{
    return m_command == DATA;
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
CybertwinHeader::SetSize(uint32_t val)
{
    m_size = val;
}

uint32_t
CybertwinHeader::GetSize() const
{
    return m_size;
}

void
CybertwinHeader::SetCybertwinPort(uint16_t val)
{
    m_cybertwinPort = val;
}

uint16_t
CybertwinHeader::GetCybertwinPort() const
{
    return m_cybertwinPort;
}

} // namespace ns3