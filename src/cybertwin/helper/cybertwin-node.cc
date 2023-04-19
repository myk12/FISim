#include "cybertwin-node.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CybertwinNode");

NS_OBJECT_ENSURE_REGISTERED (CybertwinNode);

TypeId
CybertwinNode::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CybertwinNode")
                            .SetParent<Node>()
                            .SetGroupName("Cybertwin")
                            .AddConstructor<CybertwinNode>()
                            .AddAttribute("UpperNodeAddress",
                                          "The address of the upper node.",
                                          Ipv4AddressValue(Ipv4Address("0.0.0.0")),
                                          MakeIpv4AddressAccessor(&CybertwinNode::upperNodeAddress),
                                          MakeIpv4AddressChecker());
    
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
CybertwinNode::Setup()
{
    NS_LOG_DEBUG("Setup a CybertwinNode.");
}
