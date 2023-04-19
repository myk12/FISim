#ifndef CYBERTWIN_NODE_H
#define CYBERTWIN_NODE_H

#include "../model/cybertwin-common.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

namespace ns3
{

class CybertwinNode : public Node
{
  public:
    CybertwinNode();
    ~CybertwinNode();

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const;

    virtual void Setup();

  protected:
    Ipv4Address m_upperNodeAddress; // ip address of default cybertwin controller
    Ipv4Address m_selfNodeAddress;  // ip address of the current node
    CYBERTWINID_t m_cybertwinId;
};

} // namespace ns3

#endif