#include "cybertwin-node.h"
#include "cybertwin-name-resolution-service.h"

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
}

void
CybertwinNode::AddConfigFile(std::string filename, nlohmann::json config)
{
    m_configFiles[filename] = config;
}

}// namespace ns3
