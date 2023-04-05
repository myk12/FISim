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


//********************************************************************
//*             CyberTwin Multipath Header                           *
//********************************************************************
NS_OBJECT_ENSURE_REGISTERED(MultipathHeader);

MultipathHeader::MultipathHeader() :
    m_pathId(0),
    m_cuid(0),
    m_senderKey(0),
    m_recverKey(0),
    m_connId(0)
{
}

MultipathHeader::~MultipathHeader()
{
}

TypeId MultipathHeader::GetTypeId()
{
  static TypeId tid = TypeId("ns3::MultipathHeader")
                      .SetParent<Header>()
                      .AddConstructor<MultipathHeader>();
  return tid;
}

TypeId MultipathHeader::GetInstanceTypeId() const
{
  return GetTypeId();
}

uint32_t MultipathHeader::GetSize()
{
  return sizeof(m_pathId) + sizeof(m_cuid) + sizeof(m_senderKey) + sizeof(m_recverKey) + sizeof(m_connId);
}

uint32_t MultipathHeader::GetSerializedSize() const
{
  return sizeof(m_pathId) + sizeof(m_cuid) + sizeof(m_senderKey) + sizeof(m_recverKey) + sizeof(m_connId);
}

void MultipathHeader::Serialize(Buffer::Iterator start) const
{
  start.WriteHtonU32(m_pathId);
  start.WriteHtonU64(m_cuid);
  start.WriteHtonU32(m_senderKey);
  start.WriteHtonU32(m_recverKey);
  start.WriteHtonU64(m_connId);
}

uint32_t MultipathHeader::Deserialize(Buffer::Iterator start)
{
  m_pathId = start.ReadNtohU32();
  m_cuid = start.ReadNtohU64();
  m_senderKey = start.ReadNtohU32();
  m_recverKey = start.ReadNtohU32();
  m_connId = start.ReadNtohU64();

  return GetSerializedSize();
}

void MultipathHeader::Print(std::ostream& os) const
{
  os << "------- Multipath Header -------" <<std::endl
     << "| Path ID: " << m_pathId << std::endl
     << "| CyberTwin ID: " << m_cuid << std::endl
     << "| Sender Key: " << m_senderKey << std::endl
     << "| Receiver Key: " << m_recverKey << std::endl
     << "| Connection ID: " << m_connId << std::endl
     << "--------------------------------" << std::endl;
}

MP_PATH_ID_t MultipathHeader::GetPathId() const
{
  return m_pathId;
}

void MultipathHeader::SetPathId(MP_PATH_ID_t pathId)
{
  m_pathId = pathId;
}

CYBERTWINID_t MultipathHeader::GetCuid() const
{
  return m_cuid;
}

void MultipathHeader::SetCuid(CYBERTWINID_t cuid)
{
  m_cuid = cuid;
}

MP_CONN_KEY_t MultipathHeader::GetSenderKey() const
{
  return m_senderKey;
}

void MultipathHeader::SetSenderKey(MP_CONN_KEY_t senderKey)
{
  m_senderKey = senderKey;
}

MP_CONN_KEY_t MultipathHeader::GetRecverKey() const
{
  return m_recverKey;
}

void MultipathHeader::SetRecverKey(MP_CONN_KEY_t recverKey)
{
  m_recverKey = recverKey;
}

MP_CONN_ID_t MultipathHeader::GetConnId() const
{
  return m_connId;
}

void MultipathHeader::SetConnId(MP_CONN_ID_t connId)
{
  m_connId = connId;
}

} // namespace ns3