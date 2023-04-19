#ifndef CYBERTWIN_NODE_H
#define CYBERTWIN_NODE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
class CybertwinNode : public Node
{
    public:
        CybertwinNode();
        ~CybertwinNode();

        static TypeId GetTypeId();
        TypeId GetInstanceTypeId() const;

        virtual void Setup();

    protected:
        Ipv4Address upperNodeAddress; // ip address of default cybertwin controller
};


#endif