#include "cybertwin-packet-header.h"

#include "ns3/log.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinPacketHeader");
NS_OBJECT_ENSURE_REGISTERED(CybertwinPacketHeader);

CybertwinPacketHeader::CybertwinPacketHeader(uint64_t src,
                                             uint64_t dst,
                                             uint32_t size,
                                             uint16_t cmd)
    : m_src(src),
      m_dst(dst),
      m_size(size),
      m_cmd(cmd)
{
}

TypeId
CybertwinPacketHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinPacketHeader")
                            .SetParent<Header>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinPacketHeader>();
    return tid;
}

TypeId
CybertwinPacketHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint64_t
CybertwinPacketHeader::GetSrc() const
{
    return m_src;
}

void
CybertwinPacketHeader::SetSrc(uint64_t src)
{
    m_src = src;
}

uint64_t
CybertwinPacketHeader::GetDst() const
{
    return m_dst;
}

void
CybertwinPacketHeader::SetDst(uint64_t dst)
{
    m_dst = dst;
}

uint32_t
CybertwinPacketHeader::GetSize() const
{
    return m_size;
}

void
CybertwinPacketHeader::SetSize(uint32_t size)
{
    m_size = size;
}

uint16_t
CybertwinPacketHeader::GetCmd() const
{
    return m_cmd;
}

void
CybertwinPacketHeader::SetCmd(uint16_t cmd)
{
    m_cmd = cmd;
}

void
CybertwinPacketHeader::Print(std::ostream& os) const
{
    os << "(src=" << m_src << " dst=" << m_dst << " size=" << m_size << " cmd=" << m_cmd << ")";
}

std::string
CybertwinPacketHeader::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

uint32_t
CybertwinPacketHeader::GetSerializedSize() const
{
    return 8 + 8 + 4 + 2;
}

void
CybertwinPacketHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU64(m_src);
    i.WriteHtonU64(m_dst);
    i.WriteHtonU32(m_size);
    i.WriteHtonU16(m_cmd);
}

uint32_t
CybertwinPacketHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_src = i.ReadNtohU64();
    m_dst = i.ReadNtohU64();
    m_size = i.ReadNtohU32();
    m_cmd = i.ReadNtohU16();
    return GetSerializedSize();
}

//********************************************************************
//*              Cybertwin Controller Header                         *
//********************************************************************

CybertwinControllerHeader::CybertwinControllerHeader():
    method(NOTHING),
    devName(0),
    netType(0),
    cybertwinID(0)
{
}

TypeId
CybertwinControllerHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinControllerHeader")
                            .SetParent<Header>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinControllerHeader>();
    return tid;
}

TypeId
CybertwinControllerHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
CybertwinControllerHeader::Print(std::ostream& os) const
{
    os << "(method=" << method << ")";
}

uint32_t
CybertwinControllerHeader::GetSerializedSize() const
{
    return sizeof(method)
            + sizeof(devName)
            + sizeof(netType)
            + sizeof(cybertwinID)
            + sizeof(cybertwinPort);
}

void
CybertwinControllerHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU16(method);
    i.WriteHtonU64(devName);
    i.WriteHtonU16(netType);
    i.WriteHtonU64(cybertwinID);
    i.WriteHtonU16(cybertwinPort);
}

uint32_t
CybertwinControllerHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    method = i.ReadNtohU16();
    devName = i.ReadNtohU64();
    netType = i.ReadNtohU16();
    cybertwinID = i.ReadNtohU64();
    cybertwinPort = i.ReadNtohU16();
    return GetSerializedSize();
}

std::string
CybertwinControllerHeader::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

void CybertwinControllerHeader::SetMethod(uint16_t method)
{
    this->method = method;
}

uint16_t CybertwinControllerHeader::GetMethod() const
{
    return method;
}

void CybertwinControllerHeader::SetDeviceName(DEVNAME_t devName)
{
    this->devName = devName;
}

DEVNAME_t CybertwinControllerHeader::GetDeviceName() const
{
    return devName;
}

void CybertwinControllerHeader::SetNetworkType(NETTYPE_t netType)
{
    this->netType = netType;
}

NETTYPE_t CybertwinControllerHeader::GetNetworkType() const
{
    return netType;
}

void CybertwinControllerHeader::SetCybertwinID(CYBERTWINID_t cybertwinID)
{
    this->cybertwinID = cybertwinID;
}

void CybertwinControllerHeader::SetCybertwinPort(uint16_t port)
{
    this->cybertwinPort = port;
}

uint16_t CybertwinControllerHeader::GetCybertwinPort() const
{
    return cybertwinPort;
}

CYBERTWINID_t CybertwinControllerHeader::GetCybertwinID() const
{
    return cybertwinID;
}

} // namespace ns3