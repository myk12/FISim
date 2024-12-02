#include "simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinNetworkSimulator");

NS_OBJECT_ENSURE_REGISTERED(CybertwinNetworkSimulator);

AnimationInterface globalAnim("cybertwin-network.xml");

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
    std::string topologyFile = "/home/ubuntu/FISim/FIA/Cybertwin/topology.yaml";
    std::string appFiles = "/home/ubuntu/FISim/FIA/Cybertwin/applications.yaml";
    m_topologyReader.SetFileName(topologyFile);
    m_topologyReader.SetAppFiles(appFiles);
}

CybertwinNetworkSimulator::~CybertwinNetworkSimulator()
{
    NS_LOG_FUNCTION(this);
}

void CybertwinNetworkSimulator::InputInit()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[0] Initializing the simulator...");
    // Initialize the simulator
    // create netanim object
    m_anim = CreateObject<AnimationInterface>("cybertwin-network.xml");
    m_anim->SetMobilityPollInterval(Seconds(0.25)); // set mobility poll interval
    m_anim->EnableIpv4RouteTracking("cybertwin-routing.xml", Seconds(0), Seconds(10), m_nodes, Seconds(5));

    // add netanim node resources
    m_anim->AddResource("doc/netanim_icon/core_server.png");
    m_anim->AddResource("doc/netanim_icon/edge_server.png");
    m_anim->AddResource("doc/netanim_icon/endhost_station.png");
    m_anim->AddResource("doc/netanim_icon/wireless_ap.png");
    m_anim->AddResource("doc/netanim_icon/wireless_sta.png");
}

void CybertwinNetworkSimulator::DriverCompileTopology()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[1] Reading the topology file...");
    m_nodes = m_topologyReader.Read();

    // populate routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    NS_LOG_INFO("[1] Topology file read successfully!");

    // add nodes to netanim
    NodeContainer coreNodes = m_topologyReader.GetCoreCloudNodes();
    for (uint32_t i = 0; i < coreNodes.GetN(); i++)
    {
        Ptr<CybertwinCoreServer> node = DynamicCast<CybertwinCoreServer>(coreNodes.Get(i));
        m_anim->UpdateNodeDescription(node->GetId(), "Core Cloud Server");
        // set icon
        m_anim->UpdateNodeImage(node->GetId(), 0);
    }
    NodeContainer edgeNodes = m_topologyReader.GetEdgeCloudNodes();
    for (uint32_t i = 0; i < edgeNodes.GetN(); i++)
    {
        Ptr<CybertwinEdgeServer> node = DynamicCast<CybertwinEdgeServer>(edgeNodes.Get(i));
        m_anim->UpdateNodeDescription(node->GetId(), "Edge Cloud Server");
        // set icon
        m_anim->UpdateNodeImage(node->GetId(), 1);
    }

    NodeContainer endNodes = m_topologyReader.GetEndClusterNodes();
    for (uint32_t i = 0; i < endNodes.GetN(); i++)
    {
        Ptr<CybertwinEndHost> node = DynamicCast<CybertwinEndHost>(endNodes.Get(i));
        m_anim->UpdateNodeDescription(node->GetId(), "End Host");
        // set icon
        m_anim->UpdateNodeImage(node->GetId(), 2);
    }
}

void CybertwinNetworkSimulator::DriverInstallApps()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[2] Configuring the nodes and applications...");
    m_topologyReader.InstallApplications();

    // Start all installed applications
    NodeContainer coreNodes = m_topologyReader.GetCoreCloudNodes();
    for (uint32_t i = 0; i < coreNodes.GetN(); i++)
    {
        Ptr<CybertwinCoreServer> node = DynamicCast<CybertwinCoreServer>(coreNodes.Get(i));
        node->StartAllAggregatedApps();
    }

    NodeContainer edgeNodes = m_topologyReader.GetEdgeCloudNodes();
    for (uint32_t i = 0; i < edgeNodes.GetN(); i++)
    {
        Ptr<CybertwinEdgeServer> node = DynamicCast<CybertwinEdgeServer>(edgeNodes.Get(i));
        node->StartAllAggregatedApps();
    }

    NodeContainer endNodes = m_topologyReader.GetEndClusterNodes();
    for (uint32_t i = 0; i < endNodes.GetN(); i++)
    {
        Ptr<CybertwinEndHost> node = DynamicCast<CybertwinEndHost>(endNodes.Get(i));
        node->StartAllAggregatedApps();
    }

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
    Simulator::Stop(Seconds(10));
    Simulator::Run();
    NS_LOG_INFO("[4] Simulation completed!");
}

void CybertwinNetworkSimulator::Output()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("[5] Output the simulation results...");
    Simulator::Destroy();
    NS_LOG_INFO("[5] Simulation results outputted successfully!");
}
    
} // namespace ns3

using namespace ns3;
int main(int argc, char *argv[])
{
    LogComponentEnable("CybertwinNetworkSimulator", LOG_LEVEL_INFO);
    LogComponentEnable("CybertwinTopologyReader", LOG_LEVEL_INFO);
    LogComponentEnable("CybertwinNode", LOG_LEVEL_DEBUG);
    LogComponentEnable("CybertwinAppDownloadClient", LOG_LEVEL_INFO);
    LogComponentEnable("CybertwinAppDownloadServer", LOG_LEVEL_INFO);
    LogComponentEnable("CybertwinEndHostDaemon", LOG_LEVEL_DEBUG);
    LogComponentEnable("Cybertwin", LOG_LEVEL_INFO);
    LogComponentEnable("CybertwinManager", LOG_LEVEL_DEBUG);
    LogComponentEnable("CybertwinHeader", LOG_LEVEL_INFO);
    LogComponentEnable("NameResolutionService", LOG_LEVEL_INFO);
    NS_LOG_INFO("-*-*-*-*-*-*- Starting Cybertwin Network Simulator -*-*-*-*-*-*-");

    // create the simulator
    Ptr<ns3::CybertwinNetworkSimulator> simulator = CreateObject<ns3::CybertwinNetworkSimulator>();
    
    // initiliaze the simulator
    simulator->InputInit();

    // read the topology file
    simulator->DriverCompileTopology();

    // boot the simulator
    simulator->DriverBootSimulator();

    // configure the nodes and applications
    simulator->DriverInstallApps();

    // run the simulator
    simulator->RunSimulator();

    // output the simulation results
    simulator->Output();

    return 0;

}; // namespace ns3

