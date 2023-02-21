#ifndef CYBERTWIN_COMMON_H
#define CYBERTWIN_COMMON_H
#include <string>
#include <utility>
#include "ns3/address.h"

#define TX_MAX_NUM (128)
#define LOCAL_PORT_COUNTER_START (40000)
#define GLOBAL_PORT_COUNTER_START (50000)


typedef uint64_t CYBERTWINID_t;
typedef uint64_t DEVNAME_t;
typedef uint16_t NETTYPE_t;
typedef std::pair<ns3::Address, uint16_t> CybertwinInterface;

enum{
    NOTHING,
    CYBERTWIN_CREATE,
    CYBERTWIN_REMOVE,
    CYBERTWIN_CONTROLLER_SUCCESS,
    CYBERTWIN_CONTROLLER_ERROR,
};

#endif