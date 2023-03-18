#ifndef TCP_OPTION_MPTCP_H
#define TCP_OPTION_MPTCP_H

#include "ns3/ipv4-address.h"
#include "ns3/tcp-option.h"

namespace ns3
{
/**
 * \ingroup tcp
 * 
 * Defines the TCP option of MPTCP
*/

class TcpOptionMpCapable: public TcpOption
{
public:
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    TcpOptionMpCapable();
    TcpOptionMpCapable(uint32_t);
    ~TcpOptionMpCapable();

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint8_t GetKind() const override;
    uint32_t GetSerializedSize() const override;

    void SetSenderToken(uint32_t);
    uint32_t GetSenderToken() const;
protected:
    uint32_t senderToken;
};

class TcpOptionJoinConn: public TcpOption
{
public:
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    TcpOptionJoinConn();
    TcpOptionJoinConn(uint32_t, uint8_t);
    ~TcpOptionJoinConn();

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint8_t GetKind() const override;
    uint32_t GetSerializedSize() const override;

    void SetReceiverToken(uint32_t);
    uint32_t GetReceiverToken() const;
    void SetAddrID(uint8_t);
    uint8_t GetAddrID() const;
protected:
    uint32_t receiverToken;
    uint8_t addrID;
};

class TcpOptionAddAddr: public TcpOption
{
public:
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    TcpOptionAddAddr();
    TcpOptionAddAddr(uint8_t, Ipv4Address);
    ~TcpOptionAddAddr();

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint8_t GetKind() const override;
    uint32_t GetSerializedSize() const override;

    void SetAddrID(uint8_t);
    uint8_t GetAddrID() const;
    void SetIpv4Addr(Ipv4Address);
    Ipv4Address GetIpv4Addr() const;
protected:
    uint8_t addrID;
    Ipv4Address addr;
};

class TcpOptionDataSeqMapping: public TcpOption
{
public:
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    TcpOptionDataSeqMapping();
    TcpOptionDataSeqMapping(uint64_t, uint16_t, uint32_t);
    ~TcpOptionDataSeqMapping();

    void Print(std::ostream& os) const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    uint8_t GetKind() const override;
    uint32_t GetSerializedSize() const override;

    void SetDataSeqNumber(uint64_t);
    uint64_t GetDataSeqNumber() const;
    void SetDataLevelLength(uint16_t);
    uint16_t GetDataLevelLength() const;
    void SetSubflowSeqNumber(uint32_t);
    uint32_t GetSubflowSeqNumber() const;

protected:
    uint64_t dataSeqNumber;
    uint16_t dataLevelLength;
    uint32_t subflowSeqNumber;
};

}


#endif /* TCP_OPTION_MPTCP_H */