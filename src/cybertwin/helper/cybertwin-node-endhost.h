#ifndef CYBERTWIN_NODE_ENDHOST_H
#define CYBRETWIN_NODE_ENDHOST_H

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/cybertwin-common.h"
#include "ns3/applications-module.h"

namespace ns3
{

class CybertwinEndHost: public Node
{
    public:
        CybertwinEndHost();
        ~CybertwinEndHost();

        static TypeId GetTypeId();

        int32_t Setup(Ipv4Address edgeServerAddr);

    private:
        Ipv4Address edgeServerAddress;  // ip address of default cybertwin controller
        Ptr<Application> cybertwinControllerClient;
        Ptr<Application> cybertwinClient;
};

}


#endif