#include "cybertwin-node.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CybertwinNode");
NS_OBJECT_ENSURE_REGISTERED(CybertwinNode);

TypeId
CybertwinNode::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CybertwinNode")
            .SetParent<Node>()
            .SetGroupName("Cybertwin")
            .AddConstructor<CybertwinNode>()
            .AddAttribute("UpperNodeAddress",
                          "The address of the upper node",
                          Ipv4AddressValue(),
                          MakeIpv4AddressAccessor(&CybertwinNode::m_upperNodeAddress),
                          MakeIpv4AddressChecker())
            .AddAttribute("SelfNodeAddress",
                          "The address of the current node",
                          Ipv4AddressValue(),
                          MakeIpv4AddressAccessor(&CybertwinNode::m_selfNodeAddress),
                          MakeIpv4AddressChecker())
            .AddAttribute("SelfCuid",
                          "The cybertwin id of the current node",
                          UintegerValue(),
                          MakeUintegerAccessor(&CybertwinNode::m_cybertwinId),
                          MakeUintegerChecker<uint64_t>());
    return tid;
};

TypeId
CybertwinNode::GetInstanceTypeId() const
{
    return GetTypeId();
}

CybertwinNode::CybertwinNode()
{
    NS_LOG_DEBUG("Created a CybertwinNode.");
}

CybertwinNode::~CybertwinNode()
{
    NS_LOG_DEBUG("Destroyed a CybertwinNode.");
}

void
CybertwinNode::PowerOn()
{
    NS_LOG_DEBUG("Setup a CybertwinNode.");
}

void
CybertwinNode::SetAddressList(std::vector<Ipv4Address> addressList)
{
    ipv4AddressList = addressList;
}

void
CybertwinNode::SetName(std::string name)
{
    m_name = name;
}

std::string
CybertwinNode::GetName()
{
    return m_name;
}

void
CybertwinNode::AddParent(Ptr<Node> parent)
{
    for (auto it = m_parents.begin(); it != m_parents.end(); ++it)
    {
        if (*it == parent)
        {
            return;
        }
    }
    m_parents.push_back(parent);
}

Ipv4Address
CybertwinNode::GetUpperNodeAddress()
{
    return m_upperNodeAddress;
}

void
CybertwinNode::InstallCNRSApp()
{
    NS_LOG_DEBUG("Installing CNRS app.");
    if (!m_upperNodeAddress.IsInitialized())
    {
        if (m_parents.size() == 0)
        {
            NS_LOG_WARN("No parent node found.");
            return ;
        }else
        {
            Ptr<Node> upperNode = m_parents[0];
            m_upperNodeAddress = upperNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        }
    }

    // install CNRS application
    Ptr<NameResolutionService> cybertwinCNRSApp = CreateObject<NameResolutionService>(m_upperNodeAddress);
    this->AddApplication(cybertwinCNRSApp);
    cybertwinCNRSApp->SetStartTime(Simulator::Now());
    m_cybertwinCNRSApp = cybertwinCNRSApp;
}

void
CybertwinNode::InstallCybertwinManagerApp(std::vector<Ipv4Address> localIpList, std::vector<Ipv4Address> globalIpList)
{
    NS_LOG_DEBUG("Installing Cybertwin Manager app.");
    if (!m_upperNodeAddress.IsInitialized())
    {
        if (m_parents.size() == 0)
        {
            NS_LOG_WARN("No parent node found.");
            return ;
        }else
        {
            Ptr<Node> upperNode = m_parents[0];
            m_upperNodeAddress = upperNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
        }
        
    }

    // install Cybertwin Manager application
    Ptr<CybertwinManager> cybertwinManagerApp = CreateObject<CybertwinManager>(localIpList, globalIpList);
    this->AddApplication(cybertwinManagerApp);
    cybertwinManagerApp->SetStartTime(Simulator::Now());
}

void
CybertwinNode::AddConfigFile(std::string filename, nlohmann::json config)
{
    m_configFiles[filename] = config;
}

Ptr<NameResolutionService>
CybertwinNode::GetCNRSApp()
{
    return m_cybertwinCNRSApp;
}

//***************************************************************
//*               edge server node                              *
//***************************************************************

NS_OBJECT_ENSURE_REGISTERED(CybertwinEdgeServer);

TypeId
CybertwinEdgeServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinEdgeServer")
                            .SetParent<CybertwinNode>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinEdgeServer>();

    return tid;
}

CybertwinEdgeServer::CybertwinEdgeServer()
    : m_cybertwinCNRSApp(nullptr),
      m_CybertwinManagerApp(nullptr)
{
}

CybertwinEdgeServer::~CybertwinEdgeServer()
{
}

void
CybertwinEdgeServer::PowerOn()
{
    if (m_parents.size() == 0)
    {
        NS_LOG_ERROR("Edge server should have parents.");
        return;
    }

    InstallCNRSApp();

    // install Cybertwin Controller application
    InstallCybertwinManagerApp(m_localAddrList, m_globalAddrList);
}

Ptr<CybertwinManager>
CybertwinEdgeServer::GetCtrlApp()
{
    return m_CybertwinManagerApp;
}

void
CybertwinEdgeServer::AddLocalIp(Ipv4Address localIp)
{
    m_localAddrList.push_back(localIp);
}

void
CybertwinEdgeServer::AddGlobalIp(Ipv4Address globalIp)
{
    m_globalAddrList.push_back(globalIp);
}

std::vector<Ipv4Address>
CybertwinEdgeServer::GetLocalIpList()
{
    return m_localAddrList;
}

std::vector<Ipv4Address>
CybertwinEdgeServer::GetGlobalIpList()
{
    return m_globalAddrList;
}

//***************************************************************
//*               core server node                              *
//***************************************************************
NS_OBJECT_ENSURE_REGISTERED(CybertwinCoreServer);

TypeId
CybertwinCoreServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinCoreServer")
                            .SetParent<Node>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinCoreServer>();
    return tid;
}

CybertwinCoreServer::CybertwinCoreServer()
    : cybertwinCNRSApp(nullptr)
{
    NS_LOG_DEBUG("Creating a CybertwinCoreServer.");
}

void
CybertwinCoreServer::PowerOn()
{
    if (m_parents.size() != 0)
    {
        // has parents, install CNRS application
        // only root node has no parents
        InstallCNRSApp();
    }
}

CybertwinCoreServer::~CybertwinCoreServer()
{
    NS_LOG_DEBUG("[CybertwinCoreServer] destroy CybertwinCoreServer.");
}

//***************************************************************
//*               end host node                                 *
//***************************************************************
NS_OBJECT_ENSURE_REGISTERED(CybertwinEndHost);

TypeId
CybertwinEndHost::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinEndHost")
                            .SetParent<CybertwinNode>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinEndHost>();
    return tid;
}

CybertwinEndHost::CybertwinEndHost()
{
    NS_LOG_DEBUG("[CybertwinEndHost] create CybertwinEndHost.");
}

CybertwinEndHost::~CybertwinEndHost()
{
    NS_LOG_DEBUG("[CybertwinEndHost] destroy CybertwinEndHost.");
}

void
CybertwinEndHost::PowerOn()
{
    NS_LOG_FUNCTION(GetId());

    // Create initd
    Ptr<EndHostInitd> initd = CreateObject<EndHostInitd>();
    initd->SetAttribute("ProxyAddr", Ipv4AddressValue(m_upperNodeAddress));
    //initd->SetAttribute("ProxyPort", UintegerValue(m_proxyPort));
    AddApplication(initd);
    initd->SetStartTime(Seconds(0.0));
}

void
CybertwinEndHost::SetCybertwinId(CYBERTWINID_t id)
{
    m_cybertwinId = id;
}

CYBERTWINID_t
CybertwinEndHost::GetCybertwinId()
{
    return m_cybertwinId;
}

void
CybertwinEndHost::SetCybertwinPort(uint16_t port)
{
    m_cybertwinPort = port;
}

uint16_t
CybertwinEndHost::GetCybertwinPort()
{
    return m_cybertwinPort;
}

void
CybertwinEndHost::SetCybertwinStatus(bool stat)
{
    m_isConnected = stat;
}

bool
CybertwinEndHost::GetCybertwinStatus()
{
    return m_isConnected;
}

}// namespace ns3
