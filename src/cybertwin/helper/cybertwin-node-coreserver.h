#ifndef CYBERTWIN_NODE_CORESERVER_H
#define CYBERTWIN_NODE_CORESERVER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

namespace ns3
{
class CybertwinCoreServer: public Node
{
    public:
        CybertwinCoreServer();
        ~CybertwinCoreServer();

        static TypeId GetTypeId();
        
        int32_t Setup();
    
    private:
        Ptr<Application> cybertwinCNRSApp;
};
    
} // namespace ns3



#endif