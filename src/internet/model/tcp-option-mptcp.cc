#include "tcp-option-mptcp.h"

#include "ns3/log.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("TcpOptionMultipathTCP");


//*********************************************************************
//*                     Option - Multipath Capable                    *
//*********************************************************************
TcpOptionMpCapable::TcpOptionMpCapable()
    : TcpOption()
{
}

TcpOptionMpCapable::TcpOptionMpCapable(uint32_t TxToken)
    : senderToken(TxToken)
{
}

TcpOptionMpCapable::~TcpOptionMpCapable()
{
}

NS_OBJECT_ENSURE_REGISTERED(TcpOptionMpCapable);

TypeId
TcpOptionMpCapable::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpOptionMpCapable")
                            .SetParent<TcpOption>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpOptionMpCapable>();
    return tid;
}

TypeId
TcpOptionMpCapable::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
TcpOptionMpCapable::Print(std::ostream& os) const
{
    os << "OPT_MPC {" << senderToken << "}";
}

uint32_t
TcpOptionMpCapable::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 1 + 4; // senderToken
}

void
TcpOptionMpCapable::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    i.WriteU8(GetKind()); // Kind
    i.WriteHtonU32(senderToken);
}

uint32_t
TcpOptionMpCapable::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    uint8_t readKind = i.ReadU8();
    if (readKind != GetKind())
    {
        NS_LOG_WARN("Malformed MpTCP option. wrong type");
        return 0;
    }

    senderToken = i.ReadNtohU32();
    return GetSerializedSize();
}

uint8_t
TcpOptionMpCapable::GetKind() const
{
    return TcpOption::MPC;
}

void
TcpOptionMpCapable::SetSenderToken(uint32_t TxToken)
{
    senderToken = TxToken;
}

uint32_t
TcpOptionMpCapable::GetSenderToken() const
{
    return senderToken;
}

//*********************************************************************
//*                     Option - Join Connection                      *
//*********************************************************************

NS_OBJECT_ENSURE_REGISTERED(TcpOptionJoinConn);
TcpOptionJoinConn::TcpOptionJoinConn()
    : TcpOption()
{
}

TcpOptionJoinConn::TcpOptionJoinConn(uint32_t recvToken, uint8_t addrID)
    : receiverToken(recvToken),
      addrID(addrID)
{
}

TcpOptionJoinConn::~TcpOptionJoinConn()
{
}

TypeId
TcpOptionJoinConn::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpOptionJoinConn")
                            .SetParent<TcpOption>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpOptionJoinConn>();
    return tid;
}

TypeId
TcpOptionJoinConn::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
TcpOptionJoinConn::Print(std::ostream& os) const
{
    os << "OPT_JOIN {" << receiverToken << ", " << addrID << "}";
}

uint32_t
TcpOptionJoinConn::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 1 + 4 + // receiverToken
           1;      // addrID
}

void
TcpOptionJoinConn::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    i.WriteU8(GetKind()); // Kind
    i.WriteHtonU32(receiverToken);
    i.WriteU8(addrID);
}

uint32_t
TcpOptionJoinConn::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    uint8_t readKind = i.ReadU8();
    if (readKind != GetKind())
    {
        NS_LOG_WARN("Malformed MpTCP option. wrong type");
        return 0;
    }

    receiverToken = i.ReadNtohU32();
    addrID = i.ReadU8();
    return GetSerializedSize();
}

uint8_t
TcpOptionJoinConn::GetKind() const
{
    return TcpOption::JOIN;
}

void
TcpOptionJoinConn::SetReceiverToken(uint32_t RxToken)
{
    receiverToken = RxToken;
}

uint32_t
TcpOptionJoinConn::GetReceiverToken() const
{
    return receiverToken;
}

void
TcpOptionJoinConn::SetAddrID(uint8_t addrID)
{
    addrID = addrID;
}

uint8_t
TcpOptionJoinConn::GetAddrID() const
{
    return addrID;
}

//*********************************************************************
//*                     Option - Add Address                          *
//*********************************************************************

NS_OBJECT_ENSURE_REGISTERED(TcpOptionAddAddr);
TcpOptionAddAddr::TcpOptionAddAddr()
    : TcpOption()
{
}

TcpOptionAddAddr::TcpOptionAddAddr(uint8_t addrID, Ipv4Address ipaddr)
    : addrID(addrID),
      addr(ipaddr)
{
}

TcpOptionAddAddr::~TcpOptionAddAddr()
{
}

TypeId
TcpOptionAddAddr::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpOptionAddAddr")
                            .SetParent<TcpOption>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpOptionAddAddr>();
    return tid;
}

TypeId
TcpOptionAddAddr::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
TcpOptionAddAddr::Print(std::ostream& os) const
{
    os << "OPT_ADDR {" << addrID << ", " << addr << "}";
}

uint32_t
TcpOptionAddAddr::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 1 + 4 + // ipv4 address
           1;      // addrID
}

void
TcpOptionAddAddr::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    i.WriteU8(GetKind()); // Kind
    i.WriteU8(addrID);
    i.WriteHtonU32(addr.Get());
}

uint32_t
TcpOptionAddAddr::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    uint8_t readKind = i.ReadU8();
    if (readKind != GetKind())
    {
        NS_LOG_WARN("Malformed MpTCP option. wrong type");
        return 0;
    }

    addrID = i.ReadU8();
    addr = Ipv4Address(i.ReadNtohU32());
    return GetSerializedSize();
}

uint8_t
TcpOptionAddAddr::GetKind() const
{
    return TcpOption::ADDR;
}

void
TcpOptionAddAddr::SetAddrID(uint8_t addrID)
{
    addrID = addrID;
}

uint8_t
TcpOptionAddAddr::GetAddrID() const
{
    return addrID;
}

void
TcpOptionAddAddr::SetIpv4Addr(Ipv4Address ipv4Addr)
{
    addr = ipv4Addr;
}

Ipv4Address
TcpOptionAddAddr::GetIpv4Addr() const
{
    return addr;
}

//*********************************************************************
//*                     Option - Data Sequence Mapping                *
//*********************************************************************
NS_OBJECT_ENSURE_REGISTERED(TcpOptionDataSeqMapping);
TcpOptionDataSeqMapping::TcpOptionDataSeqMapping()
    : TcpOption()
{
}

TcpOptionDataSeqMapping::TcpOptionDataSeqMapping(uint64_t dataSeq,
                                                 uint16_t levelLength,
                                                 uint32_t subflowSeq)
    : dataSeqNumber(dataSeq),
      dataLevelLength(levelLength),
      subflowSeqNumber(subflowSeq)
{
}

TcpOptionDataSeqMapping::~TcpOptionDataSeqMapping()
{
}

TypeId
TcpOptionDataSeqMapping::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpOptionDataSeqMapping")
                            .SetParent<TcpOption>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpOptionDataSeqMapping>();
    return tid;
}

TypeId
TcpOptionDataSeqMapping::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
TcpOptionDataSeqMapping::Print(std::ostream& os) const
{
    os << "OPT_ADDR {" << dataSeqNumber << ", " << dataLevelLength << ", " << subflowSeqNumber
       << "}";
}

uint32_t
TcpOptionDataSeqMapping::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 1 + 8 + // dataSeqNumber
           2 +     // dataLevelLength
           4;      // subflowSeqNumber
}

void
TcpOptionDataSeqMapping::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    i.WriteU8(GetKind()); // Kind

    i.WriteHtonU64(dataSeqNumber);
    i.WriteHtonU16(dataLevelLength);
    i.WriteHtonU32(subflowSeqNumber);
}

uint32_t
TcpOptionDataSeqMapping::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this);
    Buffer::Iterator i = start;
    uint8_t readKind = i.ReadU8();
    if (readKind != GetKind())
    {
        NS_LOG_WARN("Malformed MpTCP option. wrong type");
        return 0;
    }

    dataSeqNumber = i.ReadNtohU64();
    dataLevelLength = i.ReadNtohU16();
    subflowSeqNumber = i.ReadNtohU32();

    return GetSerializedSize();
}

uint8_t
TcpOptionDataSeqMapping::GetKind() const
{
    return TcpOption::DSN;
}

void
TcpOptionDataSeqMapping::SetDataSeqNumber(uint64_t seqNum)
{
    dataSeqNumber = seqNum;
}

uint64_t
TcpOptionDataSeqMapping::GetDataSeqNumber() const
{
    return dataSeqNumber;
}

void
TcpOptionDataSeqMapping::SetDataLevelLength(uint16_t levelLen)
{
    dataLevelLength = levelLen;
}

uint16_t
TcpOptionDataSeqMapping::GetDataLevelLength() const
{
    return dataLevelLength;
}

void
TcpOptionDataSeqMapping::SetSubflowSeqNumber(uint32_t subflowNum)
{
    subflowSeqNumber = subflowNum;
}

uint32_t
TcpOptionDataSeqMapping::GetSubflowSeqNumber() const
{
    return subflowSeqNumber;
}

} // namespace ns3