#include "cybertwin-topology-reader.h"

#include "ns3/csma-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/point-to-point-helper.h"

#include "ns3/cybertwin-manager.h"
#include "ns3/cybertwin-node.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinTopologyReader");

NS_OBJECT_ENSURE_REGISTERED(CybertwinTopologyReader);

TypeId
CybertwinTopologyReader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinTopologyReader")
                            .SetParent<TopologyReader>()
                            .SetGroupName("TopologyReader")
                            .AddConstructor<CybertwinTopologyReader>();
    return tid;
}

CybertwinTopologyReader::CybertwinTopologyReader()
{
    NS_LOG_FUNCTION(this);
}

CybertwinTopologyReader::~CybertwinTopologyReader()
{
    NS_LOG_FUNCTION(this);
}

std::string
CybertwinTopologyReader::MaskNumberToIpv4Address(std::string mask)
{
    NS_LOG_FUNCTION(this);
    std::string result;
    uint32_t maskNumber = std::stoi(mask);
    uint32_t maskValue = 0xFFFFFFFF << (32 - maskNumber);
    result = std::to_string((maskValue >> 24) & 0xFF) + "." +
             std::to_string((maskValue >> 16) & 0xFF) + "." +
             std::to_string((maskValue >> 8) & 0xFF) + "." + std::to_string(maskValue & 0xFF);
    return result;
}

void
CybertwinTopologyReader::CreateCoreCloud()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Creating core cloud...");
    for (auto nodeInfo : m_coreNodesList)
    {
        // get target
        std::string nodeName = nodeInfo->name;

        // go through all the links and create point-to-point network
        // between the core cloud nodes
        for (auto link : nodeInfo->links)
        {
            std::string target = link.target;
            std::string data_rate = link.data_rate;
            std::string delay = link.delay;
            std::string network = link.network;

            if (m_nodeInfoMap.find(target) == m_nodeInfoMap.end())
            {
                NS_LOG_ERROR("Node not found: " << target);
                continue;
            }

            if (m_links.find(nodeName + target) != m_links.end() ||
                m_links.find(target + nodeName) != m_links.end())
            {
                NS_LOG_INFO("Link already exists: " << nodeName << " <-> " << target);
                continue;
            }

            NS_LOG_INFO("Creating p2p link: " << nodeName << " <-> " << target);
            // get target node
            Ptr<Node> targetNode = m_nodeInfoMap[target]->node;

            // create point-to-point link
            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate", StringValue(data_rate));
            p2p.SetChannelAttribute("Delay", StringValue(delay));

            NetDeviceContainer devices = p2p.Install(nodeInfo->node, targetNode);
            m_coreDevices.Add(devices);

            // assign IP addresses
            Ipv4AddressHelper ipv4;
            // parese network format 10.0.0.0/24
            std::string networkPrefix = network.substr(0, network.find('/'));
            std::string networkMask =
                MaskNumberToIpv4Address(network.substr(network.find('/') + 1));
            NS_LOG_INFO("Network: " << networkPrefix << " Mask: " << networkMask);
            ipv4.SetBase(networkPrefix.c_str(), networkMask.c_str());

            Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

            // add link to the map
            m_links.insert(nodeName + target);
            m_links.insert(target + nodeName);
        }
    }
}

void
CybertwinTopologyReader::CreateEdgeCloud()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Creating edge cloud...");

    // Construct Edge Layer Topology
    // For each node in the edge layer, create point-to-point links
    // to the upper layer nodes (core cloud)
    for (auto nodeInfo : m_edgeNodesList)
    {
        std::string nodeName = nodeInfo->name;
        for (auto link : nodeInfo->links)
        {
            std::string target = link.target;
            std::string data_rate = link.data_rate;
            std::string delay = link.delay;
            std::string network = link.network;
            NS_LOG_INFO("Target: " << target);

            if (m_nodeInfoMap.find(target) == m_nodeInfoMap.end())
            {
                NS_LOG_ERROR("Node not found: " << target);
                continue;
            }

            if (m_links.find(nodeName + target) != m_links.end() ||
                m_links.find(target + nodeName) != m_links.end())
            {
                NS_LOG_INFO("Link already exists: " << nodeName << " <-> " << target);
                continue;
            }

            NS_LOG_INFO("Creating link: " << nodeName << " <-> " << target);
            // get target node
            Ptr<Node> targetNode = m_nodeInfoMap[target]->node;

            // create point-to-point link
            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate", StringValue(data_rate));
            p2p.SetChannelAttribute("Delay", StringValue(delay));

            NetDeviceContainer devices = p2p.Install(nodeInfo->node, targetNode);
            m_edgeDevices.Add(devices);

            // assign IP addresses
            Ipv4AddressHelper ipv4;
            // parese network format
            std::string networkPrefix = network.substr(0, network.find('/'));
            std::string networkMask =
                MaskNumberToIpv4Address(network.substr(network.find('/') + 1));
            NS_LOG_INFO("Network: " << networkPrefix << " Mask: " << networkMask);
            ipv4.SetBase(networkPrefix.c_str(), networkMask.c_str());

            Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

            // add link to the map
            m_links.insert(nodeName + target);
            m_links.insert(target + nodeName);
        }
    }


    // Install Basic Cybertwin Building Blocks which includes
    // 1. CT Manager: Manages the Cybertwins
    // 2. CT Agent: Manages the communication between the Cybertwins

}

void
CybertwinTopologyReader::CreateAccessNetwork()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Creating access network...");

    for (auto nodeInfo : m_endNodesList)
    {
        // get target
        std::string nodeName = nodeInfo->name;
        std::string localNetwork = nodeInfo->local_network;

        // Install CSMA devices
        CsmaHelper csma;
        NetDeviceContainer devices;
        csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
        csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
        devices = csma.Install(nodeInfo->nodes);

        // assign IP addresses
        Ipv4AddressHelper ipv4;
        // parese network format
        std::string networkPrefix = localNetwork.substr(0, localNetwork.find('/'));
        std::string networkMask =
            MaskNumberToIpv4Address(localNetwork.substr(localNetwork.find('/') + 1));
        NS_LOG_INFO("Network: " << networkPrefix << " Mask: " << networkMask);
        ipv4.SetBase(networkPrefix.c_str(), networkMask.c_str());
        ipv4.Assign(devices);

        // connect end cluster to the edge cloud
        NS_ASSERT(nodeInfo->gateways.size() > 0);
        for (auto gateway : nodeInfo->gateways)
        {
            std::string target = gateway.name;
            std::string data_rate = gateway.data_rate;
            std::string delay = gateway.delay;
            std::string network = gateway.network;
            NS_LOG_INFO("Target: " << target);

            if (m_nodeInfoMap.find(target) == m_nodeInfoMap.end())
            {
                NS_LOG_ERROR("Node not found: " << target);
                continue;
            }

            if (m_links.find(nodeName + target) != m_links.end() ||
                m_links.find(target + nodeName) != m_links.end())
            {
                NS_LOG_INFO("Link already exists: " << nodeName << " -> " << target);
                continue;
            }

            NS_LOG_INFO("Creating link: " << nodeName << " <-> " << target);
            // get target node
            Ptr<Node> targetNode = m_nodeInfoMap[target]->node;
            // create point-to-point link between the node and the gateway
            PointToPointHelper p2p;
            p2p.SetDeviceAttribute("DataRate", StringValue(data_rate));
            p2p.SetChannelAttribute("Delay", StringValue(delay));

            NetDeviceContainer devices = p2p.Install(nodeInfo->nodes.Get(0), targetNode);
            m_endDevices.Add(devices);

            // assign IP addresses
            Ipv4AddressHelper ipv4;
            // parese network format
            std::string networkPrefix = network.substr(0, network.find('/'));
            std::string networkMask =
                MaskNumberToIpv4Address(network.substr(network.find('/') + 1));
            NS_LOG_INFO("Network: " << networkPrefix << " Mask: " << networkMask);
            ipv4.SetBase(networkPrefix.c_str(), networkMask.c_str());
            Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

            // add link to the map
            m_links.insert(nodeName + target);
            m_links.insert(target + nodeName);
        }
    }
}

void
CybertwinTopologyReader::ParseNodes(const YAML::Node& nodes, const std::string& layerName)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Parsing nodes...");

    for (YAML::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        const YAML::Node& node = *it;

        NodeInfo_t* nodeInfo = new NodeInfo_t();

        if (node["name"] && node["type"])
        {
            std::string name = node["name"].as<std::string>();
            std::string type = node["type"].as<std::string>();
            nodeInfo->name = name;

            NS_LOG_INFO("Node name: " << name);
            NS_LOG_INFO("Node type: " << type);

            // for different types of nodes we need to parse different fields
            // host_server: connections
            // end_cluster: num_nodes, gateway
            if (type == "host_server")
            {
                Ptr<Node> n = nullptr;
                // create different types of nodes according to the type
                if (layerName == "Core") {
                    n = CreateObject<CybertwinCoreServer>();
                } else if (layerName == "Edge") {
                    n = CreateObject<CybertwinEdgeServer>();
                } else if (layerName == "End") {
                    n = CreateObject<CybertwinEndHost>();
                }
                nodeInfo->node = n;
                nodeInfo->type = HOST_SERVER;
                YAML::Node connections = node["connections"];
                for (YAML::const_iterator it = connections.begin(); it != connections.end(); ++it)
                {
                    const YAML::Node& connection = *it;

                    Link_t link;
                    link.target = connection["target"].as<std::string>();
                    link.data_rate = connection["data_rate"].as<std::string>();
                    link.delay = connection["delay"].as<std::string>();
                    link.network = connection["network"].as<std::string>();

                    nodeInfo->links.push_back(link);
                }
            }
            else if (type == "end_cluster")
            {
                NS_LOG_INFO("Node type: END_CLUSTER");
                nodeInfo->type = END_CLUSTER;
                nodeInfo->num_nodes = node["num_nodes"].as<int32_t>();
                nodeInfo->local_network = node["local_network"].as<std::string>();

                for (int i = 0; i < nodeInfo->num_nodes; i++)
                {
                    Ptr<Node> n = CreateObject<CybertwinEndHost>();
                    nodeInfo->nodes.Add(n);
                    m_nodes.Add(n);
                }

                // parse gateways
                YAML::Node gateways = node["gateways"];
                for (YAML::const_iterator it = gateways.begin(); it != gateways.end(); ++it)
                {
                    const YAML::Node& gateway = *it;

                    Gateway_t gw;
                    gw.name = gateway["target"].as<std::string>();
                    gw.data_rate = gateway["data_rate"].as<std::string>();
                    gw.delay = gateway["delay"].as<std::string>();
                    gw.network = gateway["network"].as<std::string>();

                    nodeInfo->gateways.push_back(gw);
                }
            }
            else
            {
                NS_LOG_ERROR("Unknown node type: " << type);
            }
        }

        if (layerName == "Core")
        {
            m_coreNodesList.push_back(nodeInfo);
        }
        else if (layerName == "Edge")
        {
            m_edgeNodesList.push_back(nodeInfo);
        }
        else if (layerName == "End")
        {
            m_endNodesList.push_back(nodeInfo);
        }

        m_nodeInfoMap[nodeInfo->name] = nodeInfo;
    }
}

void
CybertwinTopologyReader::ParseLayer(const YAML::Node& layerNode, const std::string& layerName)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Parsing " << layerName << " layer...");

    if (layerNode["nodes"])
    {
        ParseNodes(layerNode["nodes"], layerName);
    }
    else
    {
        NS_LOG_ERROR("No nodes found in " << layerName << " layer!");
    }
}

NodeContainer
CybertwinTopologyReader::Read()
{
    NS_LOG_FUNCTION(this);

    YAML::Node topology_yaml = YAML::LoadFile(GetFileName());

    // read nodes from the topology file
    NS_ASSERT(topology_yaml["cybertwin_network"]);
    const YAML::Node& cybertwin_network = topology_yaml["cybertwin_network"];
    NS_ASSERT(cybertwin_network["core_layer"] || cybertwin_network["edge_layer"] ||
              cybertwin_network["access_layer"]);

    // parse core, edge and end layers
    ParseLayer(cybertwin_network["core_layer"], "Core");
    ParseLayer(cybertwin_network["edge_layer"], "Edge");
    ParseLayer(cybertwin_network["access_layer"], "End");

    // Install Internet stack on all nodes
    InternetStackHelper internet;
    internet.Install(m_nodes);

    // create core cloud
    CreateCoreCloud();

    // create edge cloud
    CreateEdgeCloud();

    // create access network
    CreateAccessNetwork();

    // read applications

    return m_nodes;
}

} // namespace ns3