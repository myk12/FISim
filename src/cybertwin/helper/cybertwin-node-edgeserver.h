#ifndef CYBERTWIN_NODE_EDGESERVER_H
#define CYBERTWIN_NODE_EDGESERVER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "cybertwin-node.h"
#include "../model/cybertwin-name-resolution-service.h"
#include "../model/cybertwin-edge.h"

namespace ns3
{
class CybertwinController;
class CybertwinEdgeServer: public CybertwinNode
{
    public:
        CybertwinEdgeServer();
        ~CybertwinEdgeServer();

        void Setup();

        static TypeId GetTypeId();

        Ptr<NameResolutionService> GetCNRSApp();

    private:
        Ipv4Address CNRSUpNodeAddress;
        Ptr<NameResolutionService> cybertwinCNRSApp;
        Ptr<CybertwinController> cybertwinControllerApp;
};
}

#endif