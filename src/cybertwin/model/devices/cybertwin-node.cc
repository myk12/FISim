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
CybertwinNode::InstallCybertwinManagerApp()
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
    Ptr<CybertwinManager> cybertwinManagerApp = CreateObject<CybertwinManager>();
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
    InstallCybertwinManagerApp();

    m_CybertwinManagerApp = CreateObject<CybertwinManager>();
    m_CybertwinManagerApp->SetAttribute("LocalAddress", AddressValue(m_selfNodeAddress));
    AddApplication(m_CybertwinManagerApp);
    m_CybertwinManagerApp->SetStartTime(Seconds(0.0));
}

Ptr<CybertwinManager>
CybertwinEdgeServer::GetCtrlApp()
{
    return m_CybertwinManagerApp;
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
    //m_connClient = CreateObject<CybertwinConnClient>();
    //m_bulkClient = CreateObject<CybertwinBulkClient>();
}

/*
void
CybertwinEndHost::Connect(const CybertwinCertTag& cert)
{
    NS_LOG_FUNCTION(GetId());
    m_connClient->SetAttribute("LocalAddress", AddressValue(m_selfNodeAddress));
    m_connClient->SetAttribute("EdgeAddress", AddressValue(m_upperNodeAddress));
    m_connClient->SetAttribute("LocalCuid", UintegerValue(m_cybertwinId));
    m_connClient->SetCertificate(cert);
    AddApplication(m_connClient);
    m_connClient->SetStartTime(Seconds(0));
}

void
CybertwinEndHost::SendTo(CYBERTWINID_t peer, uint32_t size)
{
    NS_LOG_FUNCTION(GetId() << peer << size);
    // TODO: will this still work when peer changes?
    m_bulkClient->SetAttribute("PeerCuid", UintegerValue(peer));
    m_bulkClient->SetAttribute("MaxBytes", UintegerValue(size));
    AddApplication(m_bulkClient);
    m_bulkClient->SetStartTime(Seconds(0));
    m_bulkClient->SetStopTime(Seconds(NORMAL_SIM_SECONDS));
}
*/

}// namespace ns3
