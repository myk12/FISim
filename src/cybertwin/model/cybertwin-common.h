#ifndef CYBERTWIN_COMMON_H
#define CYBERTWIN_COMMON_H
#include <string>
#include <utility>
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"

#define TX_MAX_NUM (128)
#define DEFAULT_PAYLOAD_LEN (128)
#define LOCAL_PORT_COUNTER_START (40000)
#define GLOBAL_PORT_COUNTER_START (50000)

#define NAME_RESOLUTION_SERVICE_PORT (5353)
#define CYBERTWIN_EDGESERVER_CONTROLLER_PORT (2323)         //Tranportation Layer cybertwin controller server port.


typedef uint64_t CYBERTWINID_t;
typedef uint64_t DEVNAME_t;
typedef uint16_t NETTYPE_t;
typedef std::pair<ns3::Address, uint16_t> CybertwinInterface;
typedef std::pair<ns3::Ipv4Address, uint16_t> CYBERTWIN_INTERFACE_t;
typedef std::vector<CYBERTWIN_INTERFACE_t> CYBERTWIN_INTERFACE_LIST_t;

enum{
    NOTHING,
    CYBERTWIN_CREATE,
    CYBERTWIN_REMOVE,
    CYBERTWIN_CONTROLLER_SUCCESS,
    CYBERTWIN_CONTROLLER_ERROR,
};

enum CNRS_METHOD
{
    CNRS_QUERY,
    CNRS_INSERT,
    CNRS_QUERY_OK,
    CNRS_QUERY_FAIL,
    CNRS_INSERT_OK,
    CNRS_INSERT_FAIL
};

#endif