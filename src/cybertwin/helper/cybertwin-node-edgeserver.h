#ifndef CYBERTWIN_NODE_EDGESERVER_H
#define CYBERTWIN_NODE_EDGESERVER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

namespace ns3
{
class CybertwinEdgeServer: public Node
{
    public:
        CybertwinEdgeServer();
        ~CybertwinEdgeServer();

        int32_t Setup(Ipv4Address upNodeAddress);

        static TypeId GetTypeId();
    
    private:
        Ipv4Address CNRSUpNodeAddress;
        Ptr<Application> cybertwinCNRSApp;
        Ptr<Application> cybertwinControllerApp;

};
}

#endif