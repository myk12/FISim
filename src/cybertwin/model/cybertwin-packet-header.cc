#include "cybertwin-packet-header.h"

#include "ns3/log.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CybertwinPacketHeader");
NS_OBJECT_ENSURE_REGISTERED(CybertwinPacketHeader);

//********************************************************************
//*                  Cybertwin Packet Header                         *
//********************************************************************

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

CybertwinControllerHeader::CybertwinControllerHeader()
    : method(NOTHING),
      cybertwinPort(0),
      isOsUpdated(0),
      isPatchUpdated(0)
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
    os << "(method=" << method << ", " << cybertwinId << ", " << cybertwinPort << ", "
       << isOsUpdated << ", " << isPatchUpdated << ")";
}

uint32_t
CybertwinControllerHeader::GetSerializedSize() const
{
    return sizeof(method) + sizeof(cybertwinId) + sizeof(cybertwinPort) + sizeof(isOsUpdated) +
           sizeof(isPatchUpdated);
}

void
CybertwinControllerHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU16(method);
    i.WriteHtonU64(cybertwinId);
    i.WriteHtonU16(cybertwinPort);
    i.WriteU8(isOsUpdated);
    i.WriteU8(isPatchUpdated);
}

uint32_t
CybertwinControllerHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    method = i.ReadNtohU16();
    cybertwinId = i.ReadNtohU64();
    cybertwinPort = i.ReadNtohU16();
    isOsUpdated = i.ReadU8();
    isPatchUpdated = i.ReadU8();
    return GetSerializedSize();
}

std::string
CybertwinControllerHeader::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

void
CybertwinControllerHeader::SetMethod(uint16_t method)
{
    this->method = method;
}

uint16_t
CybertwinControllerHeader::GetMethod() const
{
    return method;
}

void
CybertwinControllerHeader::SetCybertwinId(CYBERTWINID_t id)
{
    cybertwinId = id;
}

CYBERTWINID_t
CybertwinControllerHeader::GetCybertwinId() const
{
    return cybertwinId;
}

void
CybertwinControllerHeader::SetCybertwinPort(uint16_t port)
{
    this->cybertwinPort = port;
}

uint16_t
CybertwinControllerHeader::GetCybertwinPort() const
{
    return cybertwinPort;
}

void
CybertwinControllerHeader::SetIsOsUpdated(uint8_t os)
{
    isOsUpdated = os;
}

uint8_t
CybertwinControllerHeader::GetIsOsUpdated() const
{
    return isOsUpdated;
}

void
CybertwinControllerHeader::SetIsPatchUpdated(uint8_t patch)
{
    isPatchUpdated = patch;
}

uint8_t
CybertwinControllerHeader::GetIsPatchUpdated() const
{
    return isPatchUpdated;
}

//********************************************************************
//*                    Cybertwin CNRS Header                         *
//********************************************************************
TypeId
CybertwinCNRSHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinCNRSHeader")
                            .SetParent<Header>()
                            .SetGroupName("cybertwin")
                            .AddConstructor<CybertwinCNRSHeader>();
    return tid;
}

TypeId
CybertwinCNRSHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

CybertwinCNRSHeader::CybertwinCNRSHeader()
{
}

void
CybertwinCNRSHeader::Print(std::ostream& os) const
{
    os << "\nCybertwinID   : " << cybertwinID << "\nCybertwinAddr : " << cybertwinAddr
       << "\nCybertwinPort : " << cybertwinPort << std::endl;
}

uint32_t
CybertwinCNRSHeader::GetSerializedSize() const
{
    return sizeof(method) + sizeof(cybertwinID) + sizeof(cybertwinAddr) + sizeof(cybertwinPort);
}

void
CybertwinCNRSHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU16(method); // request type or response type
    i.WriteHtonU64(cybertwinID);    // CybertwinID

    // only serialize when response
    if (method == CNRS_QUERY_OK)
    {
        uint32_t num = interface_list.size();
        i.WriteU8(num);
        for (uint32_t idx=0; idx<num; idx++)
        {
            i.WriteHtonU32(interface_list[idx].first.Get());
            i.WriteHtonU16(interface_list[idx].second);
        }
    }
}

uint32_t
CybertwinCNRSHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    method = i.ReadNtohU16();
    cybertwinID = i.ReadNtohU64();

    if (method == CNRS_QUERY_OK)
    {
        uint32_t num = i.ReadU8();
        for (uint32_t idx=0; idx < num; idx++)
        {
            // read a inteface
            Ipv4Address ip(i.ReadNtohU32());
            uint16_t port = i.ReadNtohU16();
            interface_list.push_back(std::make_pair(ip, port));
        }
        interface_num = interface_list.size();
    }
    return GetSerializedSize();
}

std::string
CybertwinCNRSHeader::ToString() const
{
    std::ostringstream oss;
    Print(oss);
    return oss.str();
}

void
CybertwinCNRSHeader::SetMethod(uint16_t method)
{
    this->method = method;
}

uint16_t
CybertwinCNRSHeader::GetMethod()
{
    return method;
}

void
CybertwinCNRSHeader::SetCybertwinID(CYBERTWINID_t id)
{
    this->cybertwinID = id;
}

CYBERTWINID_t
CybertwinCNRSHeader::GetCybertwinID() const
{
    return cybertwinID;
}

void 
CybertwinCNRSHeader::AddCybertwinInterface(Ipv4Address ip, uint16_t port)
{
    NS_LOG_FUNCTION(this);
    interface_list.push_back(std::make_pair(ip, port));
    interface_num = interface_list.size();
}

void
CybertwinCNRSHeader::AddCybertwinInterface(CYBERTWIN_INTERFACE_LIST_t interfaces)
{
    NS_LOG_FUNCTION(this);
    for (auto interface:interfaces)
    {
        interface_list.push_back(interface);
    }
    interface_num = interfaces.size();
}

uint16_t
CybertwinCNRSHeader::GetCybertwinInterfaceNum()
{
    return interface_list.size();
}

CYBERTWIN_INTERFACE_t
CybertwinCNRSHeader::GetCybertwinInterface(uint32_t i)
{
    NS_ASSERT(0 <= i && i < interface_list.size());
    return interface_list[i];
}

CYBERTWIN_INTERFACE_LIST_t
CybertwinCNRSHeader::GetCybertwinInterface()
{
    return interface_list;
}

} // namespace ns3