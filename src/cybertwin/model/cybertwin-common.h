#ifndef CYBERTWIN_COMMON_H
#define CYBERTWIN_COMMON_H

#include "ns3/address.h"
#include "ns3/network-module.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/sequence-number.h"

#include <string>
#include <unordered_map>
#include <utility>

using namespace ns3;

#define TX_MAX_NUM (128)
#define DEFAULT_PAYLOAD_LEN (128)
#define LOCAL_PORT_COUNTER_START (40000)
#define GLOBAL_PORT_COUNTER_START (50000)

#define NAME_RESOLUTION_SERVICE_PORT (5353)
#define CYBERTWIN_EDGESERVER_CONTROLLER_PORT (2323)         //Tranportation Layer cybertwin controller server port.

#define SP_KEYS_TO_CONNEID(connid, key1, key2)\
do {\
    connid = key1;  \
    connid = connid << 32;   \
    connid += key2;  \
}while(0)

typedef uint64_t CYBERTWINID_t;
typedef uint64_t DEVNAME_t;
typedef uint16_t NETTYPE_t;
typedef uint64_t MP_CONN_ID_t;
typedef uint32_t MP_CONN_KEY_t;
typedef uint32_t MP_PATH_ID_t;
typedef SequenceNumber<uint64_t, uint64_t> MpDataSeqNum;
typedef std::pair<ns3::Address, uint16_t> CybertwinInterface;
typedef std::pair<ns3::Ipv4Address, uint16_t> CYBERTWIN_INTERFACE_t;
typedef std::vector<CYBERTWIN_INTERFACE_t> CYBERTWIN_INTERFACE_LIST_t;

static std::unordered_map<CYBERTWINID_t, ns3::Address> GlobalRouteTable;

enum CNRS_METHOD
{
    CNRS_QUERY,
    CNRS_INSERT,
    CNRS_QUERY_OK,
    CNRS_QUERY_FAIL,
    CNRS_INSERT_OK,
    CNRS_INSERT_FAIL
};

namespace ns3
{

struct AddressHash
{
    size_t operator()(const Address& x) const
    {
        if (InetSocketAddress::IsMatchingType(x))
        {
            InetSocketAddress a = InetSocketAddress::ConvertFrom(x);
            return Ipv4AddressHash()(a.GetIpv4());
        }
        else if (Inet6SocketAddress::IsMatchingType(x))
        {
            Inet6SocketAddress a = Inet6SocketAddress::ConvertFrom(x);
            return Ipv6AddressHash()(a.GetIpv6());
        }

        NS_ABORT_MSG("PacketSink: unexpected address type, neither IPv4 nor IPv6");
        return 0;
    }
};

void DoSocketMethod(int (Socket::*)(const Address&), Ptr<Socket>, const Address&, uint16_t);
uint16_t DoSocketBind(Ptr<Socket>, const Address&);
uint16_t GetBindPort(Ptr<Socket>);

} // namespace ns3

#endif