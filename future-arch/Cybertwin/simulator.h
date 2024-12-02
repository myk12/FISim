#ifndef CYBERTWIN_SIMULATOR_H
#define CYBERTWIN_SIMULATOR_H

#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/cybertwin-topology-reader.h"
#include "ns3/netanim-module.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/node-container.h"
#include "ns3/cybertwin-topology-reader.h"
#include "ns3/ipv4-global-routing-helper.h"
namespace ns3
{
class CybertwinNetworkSimulator : public Object
{
public:
    static TypeId GetTypeId();

    CybertwinNetworkSimulator();
    ~CybertwinNetworkSimulator() override;

    CybertwinNetworkSimulator(const CybertwinNetworkSimulator &) = delete;
    CybertwinNetworkSimulator &operator=(const CybertwinNetworkSimulator &) = delete;

    void InputInit();
    void DriverCompileTopology();
    void DriverInstallApps();
    void DriverBootSimulator();
    void RunSimulator();
    void Output();

private:
    NodeContainer m_nodes;
    CybertwinTopologyReader m_topologyReader;
    Ptr<AnimationInterface> m_anim;
};

}; // namespace ns3

#endif /* CYBERTWIN_SIMULATOR_H */