#include "ns3/cybertwin-app-helper.h"

namespace ns3
{
    NS_LOG_COMPONENT_DEFINE("CybertwinAppHelper");

    CybertwinAppHelper::CybertwinAppHelper()
    {
        NS_LOG_FUNCTION(this);
    }

    CybertwinAppHelper::~CybertwinAppHelper()
    {
        NS_LOG_FUNCTION(this);
    }

    void CybertwinAppHelper::InstallApplications(std::string name, YAML::Node paramters, NodeContainer nodes)
    {
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("Installing applications on " << name << " nodes...");
        NS_LOG_INFO("Parameters: " << paramters);

        //for (int i = 0; i < nodes.GetN(); i++)
        //{
        //    Ptr<Node> node = nodes.Get(i);
        //    Ptr<CybertwinNode> cybertwinNode = DynamicCast<CybertwinNode>(node);
        //    cybertwinNode->InstallApplications();
        //}
    }
} // namespace ns3
