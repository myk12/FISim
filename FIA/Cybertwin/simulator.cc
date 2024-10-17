#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/node-container.h"
#include "ns3/cybertwin-topology-reader.h"
#include "ns3/netanim-module.h"
#include "simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinNetworkSimulator");

NS_OBJECT_ENSURE_REGISTERED(CybertwinNetworkSimulator);

TypeId
CybertwinNetworkSimulator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinNetworkSimulator")
                            .SetParent<Object>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinNetworkSimulator>();
    return tid;
}

CybertwinNetworkSimulator::CybertwinNetworkSimulator()
{
    NS_LOG_FUNCTION(this);
    m_topologyReader.SetFileName("./FIA/Cybertwin/topology.yaml");
    m_topologyReader.SetAppFiles("./FIA/Cybertwin/applications.yaml");
}

CybertwinNetworkSimulator::~CybertwinNetworkSimulator()
{
    NS_LOG_FUNCTION(this);
}

void CybertwinNetworkSimulator::DriverCompileTopology()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[1] Reading the topology file...");
    m_nodes = m_topologyReader.Read();
    NS_LOG_INFO("[1] Topology file read successfully!");
}

void CybertwinNetworkSimulator::DriverInstallApps()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[2] Configuring the nodes and applications...");
    m_topologyReader.InstallApplications();
    NS_LOG_INFO("[2] Nodes and applications configured successfully!");
}

void CybertwinNetworkSimulator::DriverBootSimulator()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[3] Running the simulation...");
    // boot the Cybertwin Network
    // 1. Power on Cloud Nodes
    // 2. Power on Edge Nodes
    // 3. Power on End Hosts

    // power on the nodes
    NodeContainer coreNodes = m_topologyReader.GetCoreCloudNodes();
    for (uint32_t i = 0; i < coreNodes.GetN(); i++)
    {
        Ptr<CybertwinCoreServer> node = DynamicCast<CybertwinCoreServer>(coreNodes.Get(i));
        node->PowerOn();
    }

    NodeContainer edgeNodes = m_topologyReader.GetEdgeCloudNodes();
    for (uint32_t i = 0; i < edgeNodes.GetN(); i++)
    {
        Ptr<CybertwinEdgeServer> node = DynamicCast<CybertwinEdgeServer>(edgeNodes.Get(i));
        node->PowerOn();
    }

    NodeContainer endNodes = m_topologyReader.GetEndClusterNodes();
    for (uint32_t i = 0; i < endNodes.GetN(); i++)
    {
        Ptr<CybertwinEndHost> node = DynamicCast<CybertwinEndHost>(endNodes.Get(i));
        node->PowerOn();
    }

    NS_LOG_INFO("[3] Simulation completed!");
}

void CybertwinNetworkSimulator::RunSimulator()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[4] Running the simulation...");
    Simulator::Run();
    NS_LOG_INFO("[4] Simulation completed!");
}

void CybertwinNetworkSimulator::Output()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[5] Output the simulation results...");
    NS_LOG_INFO("[5] Simulation results outputted successfully!");
}
    
} // namespace ns3

using namespace ns3;
int main(int argc, char *argv[])
{
    LogComponentEnable("CybertwinNetworkSimulator", LOG_LEVEL_INFO);
    LogComponentEnable("CybertwinTopologyReader", LOG_LEVEL_INFO);
    LogComponentEnable("CybertwinNode", LOG_LEVEL_INFO);
    LogComponentEnable("DownloadServer", LOG_LEVEL_INFO);
    LogComponentEnable("DownloadClient", LOG_LEVEL_INFO);
    LogComponentEnable("EndHostInitd", LOG_LEVEL_INFO);
    NS_LOG_INFO("-*-*-*-*-*-*- Starting Cybertwin Network Simulator -*-*-*-*-*-*-");

    // create the simulator
    Ptr<ns3::CybertwinNetworkSimulator> simulator = CreateObject<ns3::CybertwinNetworkSimulator>();

    // read the topology file
    simulator->DriverCompileTopology();

    // configure the nodes and applications
    simulator->DriverInstallApps();

    // boot the simulator
    simulator->DriverBootSimulator();

    // enable netanim
    AnimationInterface anim("cybertwin.xml");

    // run the simulator
    simulator->RunSimulator();

    // output the simulation results
    simulator->Output();

    return 0;

}; // namespace ns3


