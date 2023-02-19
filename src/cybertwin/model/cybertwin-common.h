#ifndef CYBERTWIN_COMMON_H
#define CYBERTWIN_COMMON_H
#include <string>
#include <utility>
#include "ns3/address.h"

#define TX_MAX_NUM (128)

typedef uint64_t GUID_t;
typedef std::pair<ns3::Address, uint16_t> CybertwinInterface;

enum{
    NOTHING,
    CREATE,
    REMOVE,
};

#endif