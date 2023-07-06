#include "CybertwinSim.h"
#include <iostream>
#include <filesystem>

namespace ns3 {
namespace fs = std::filesystem;

NS_LOG_COMPONENT_DEFINE("CybertwinSim");

CybertwinSim::CybertwinSim()
{
    m_coreCloudNodes.Create(0);
    m_edgeCloudNodes.Create(0);
    m_endHostNodes.Create(0);
}

CybertwinSim::~CybertwinSim()
{
}

int32_t CybertwinSim::Compiler()
{
    ParseNodes();
    InitTopology();
    return 0;
}

int32_t CybertwinSim::InitTopology()
{   
    InternetStackHelper stack;
    // install network stack for each nodes
    stack.Install(m_coreCloudNodes);
    stack.Install(m_edgeCloudNodes);
    stack.Install(m_endHostNodes);

    // parse topolgoy
    nlohmann::json topology;
    nlohmann::json edgeConf;
    nlohmann::json accessConf;
    std::ifstream topologyFile(CYBERTWIN_TOPOLOGY_FILE);
    topologyFile >> topology;
    topologyFile.close();

    edgeConf = topology["edge_cloud"];
    accessConf = topology["access_net"];

    // connect core servers
    NS_LOG_INFO("Connecting core servers.");
    NetDeviceContainer devices;
    PointToPointHelper p2pHelper;
    Ipv4AddressHelper address;
    char ipBase[128] = {0};

    p2pHelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2pHelper.SetChannelAttribute("Delay", StringValue("2ms"));

    for (uint32_t i=1; i<m_coreCloudNodes.GetN(); i++)
    {
        Ptr<Node> p = m_coreCloudNodes.Get(i-1);
        Ptr<Node> q = m_coreCloudNodes.Get(i);

        devices = p2pHelper.Install(p, q);

        bzero(ipBase, sizeof(ipBase));
        sprintf(ipBase, CORE_CLOUD_DEFAULT_IP_FORMAT, i);
        address.SetBase(ipBase, CORE_CLOUD_DEFAULT_MASK);
        address.Assign(devices);
    }

    // connect edge servers
    NS_LOG_INFO("Connecting edge servers.");
    p2pHelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2pHelper.SetChannelAttribute("Delay", StringValue("10ms"));

    // do for each edge server
    int net_id = 0;
    for (auto& it:edgeConf.items())
    {
        nlohmann::json server = it.value();
        // get parent node
        std::string nodeName = EDGE_CLOUD_NODE_PREFIX + it.key();
        std::string linkType = server["link_type"];

        Ptr<Node> edgeNode = m_nodes.GetNodeByName(nodeName);
        if (edgeNode == nullptr)
        {
            NS_LOG_ERROR("Edge node does not exist");
            return -1;
        }

        // connect to parent node
        nlohmann::json parents = server["parents"];
        for (auto parent:parents)
        {
            std::string parentName = parent.get<std::string>();
            Ptr<Node> parentNode = m_nodes.GetNodeByName(parentName);
            if (parentNode == nullptr)
            {
                NS_LOG_ERROR("Parent node does not exist");
                return -1;
            }
            NS_LOG_INFO("Connecting edge server " << nodeName << " to parent " << parentName << ".");

            if (linkType == "p2p")
            {
                devices = p2pHelper.Install(parentNode, edgeNode);
                bzero(ipBase, sizeof(ipBase));
                snprintf(ipBase, sizeof(ipBase), EDGE_CLOUD_DEFAULT_IP_FORMAT, net_id++);
                address.SetBase(ipBase, EDGE_CLOUD_DEFAULT_MASK);
                address.Assign(devices);
            }
            else if (linkType == "csma")
            {
                NS_LOG_ERROR("CSMA is not supported yet");
                return -1;
                /*
                CsmaHelper csmaHelper;
                csmaHelper.SetChannelAttribute("DataRate", StringValue("10Mbps"));
                csmaHelper.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
                csmaHelper.SetDeviceAttribute("Mtu", UintegerValue(1500));
                csmaHelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
                csmaHelper.SetDeviceAttribute("Delay", TimeValue(NanoSeconds(6560)));

                devices = csmaHelper.Install(parentNode, edgeNode);
                */
            }
            else
            {
                NS_LOG_ERROR("Unknown link type");
                return -1;
            }
        }

    }

    // do for each end host
    net_id = 0;
    for (auto host:accessConf.items())
    {
        // get parent node
        nlohmann::json endhost = host.value();
        nlohmann::json parents = endhost["parents"];
        std::string linkType = endhost["link_type"];
        std::string nodeName = ACCESS_NET_NODE_PREFIX + host.key();

        Ptr<Node> endHost = m_nodes.GetNodeByName(nodeName);
        if (endHost == nullptr)
        {
            NS_LOG_ERROR("End host does not exist");
            return -1;
        }

        // connect to parent node
        for (auto parent:parents)
        {
            std::string parentName = parent.get<std::string>();
            Ptr<Node> parentNode = m_nodes.GetNodeByName(parentName);
            if (parentNode == nullptr)
            {
                NS_LOG_ERROR("Parent node does not exist");
                return -1;
            }

            NS_LOG_INFO("Connecting end host " << nodeName << " to parent " << parentName << ".");

            if (linkType == "p2p")
            {
                devices = p2pHelper.Install(parentNode, endHost);
                bzero(ipBase, sizeof(ipBase));
                snprintf(ipBase, sizeof(ipBase), ACCESS_NET_DEFAULT_IP_FORMAT, net_id++);
                address.SetBase(ipBase, ACCESS_NET_DEFAULT_MASK);
                address.Assign(devices);
            }
            else if (linkType == "csma")
            {
                NS_LOG_ERROR("CSMA is not supported yet");
                return -1;
                /*
                CsmaHelper csmaHelper;
                csmaHelper.SetChannelAttribute("DataRate", StringValue("10Mbps"));
                csmaHelper.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
                csmaHelper.SetDeviceAttribute("Mtu", UintegerValue(1500));
                csmaHelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
                csmaHelper.SetDeviceAttribute("Delay", TimeValue(NanoSeconds(6560)));

                devices = csmaHelper.Install(parentNode, endHost);
                */
            }
            else
            {
                NS_LOG_ERROR("Unknown link type");
                return -1;
            }
        }

    }

    return 0;

}

int32_t CybertwinSim::ParseNodes()
{
   NS_LOG_INFO("Cybertwin::InitTopology()"); 

   // parse nodes
   // 1. core cloud
   if (!fs::exists(CORE_CLOUD_CONF_PATH) || !fs::is_directory(CORE_CLOUD_CONF_PATH))
   {
       NS_LOG_ERROR("Core cloud configuration path does not exist or is not a directory");
       return -1;
   }

   for (const auto& entry : fs::directory_iterator(CORE_CLOUD_CONF_PATH))
   {
        if (fs::is_directory(entry))
        {
            std::string nodeName = CORE_CLOUD_NODE_PREFIX + entry.path().filename().string();
            NS_LOG_INFO("Core cloud node: " << nodeName);
            Ptr<CybertwinCoreServer> coreServer = CreateObject<CybertwinCoreServer>();
            
            m_nodes.AddNode(nodeName, coreServer);
            m_coreCloudNodes.Add(coreServer);

            coreServer->SetName(nodeName);
            coreServer->Setup();
        }
   }

   // 2. edge cloud
    if (!fs::exists(EDGE_CLOUD_CONF_PATH) || !fs::is_directory(EDGE_CLOUD_CONF_PATH))
    {
         NS_LOG_ERROR("Edge cloud configuration path does not exist or is not a directory");
         return -1;
    }

    for (const auto& entry : fs::directory_iterator(EDGE_CLOUD_CONF_PATH))
    {
         if (fs::is_directory(entry))
         {
             std::string nodeName = EDGE_CLOUD_NODE_PREFIX + entry.path().filename().string();
             NS_LOG_INFO("Edge cloud node: " << nodeName);
             Ptr<CybertwinEdgeServer> edgeServer = CreateObject<CybertwinEdgeServer>();

             m_nodes.AddNode(nodeName, edgeServer);
             m_edgeCloudNodes.Add(edgeServer);
             
             edgeServer->SetName(nodeName);
             edgeServer->Setup();
         }
    }

    // 3. access network
    if (!fs::exists(ACCESS_NET_CONF_PATH) || !fs::is_directory(ACCESS_NET_CONF_PATH))
    {
         NS_LOG_ERROR("Access network configuration path does not exist or is not a directory");
         return -1;
    }

    for (const auto& entry : fs::directory_iterator(ACCESS_NET_CONF_PATH))
    {
         if (fs::is_directory(entry))
         {
             std::string nodeName = ACCESS_NET_NODE_PREFIX + entry.path().filename().string();
             NS_LOG_INFO("Access network node: " << nodeName);
             Ptr<CybertwinEndHost> endHost = CreateObject<CybertwinEndHost>();

             m_nodes.AddNode(nodeName, endHost);
             m_endHostNodes.Add(endHost);

             endHost->SetName(nodeName);
             endHost->Setup();
         }
    }

    return 0;
}


//**********************************************************************
//*                       CybertwinNodeContainer                       *
//**********************************************************************
CybertwinNodeContainer::CybertwinNodeContainer()
{
}

CybertwinNodeContainer::~CybertwinNodeContainer()
{
}

Ptr<Node> CybertwinNodeContainer::GetNodeByName(std::string name)
{
    for (auto node:m_nodes)
    {
        if (node.nameStr == name)
        {
            return node.nodePtr;
        }
    }

    return nullptr;
}

int32_t CybertwinNodeContainer::AddNode(std::string name, Ptr<Node> node)
{
    for (auto node:m_nodes)
    {
        if (node.nameStr == name)
        {
            NS_LOG_ERROR("Node " << name << " already exists");
            return -1;
        }
    }

    NodeInfo nodeInfo;
    nodeInfo.nameStr = name;
    nodeInfo.nodePtr = node;
    m_nodes.push_back(nodeInfo);

    return 0;
}

} // namespace ns3