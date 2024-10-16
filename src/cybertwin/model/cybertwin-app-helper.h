#ifndef _CYBERTWIN_APP_HELPER_H_
#define _CYBERTWIN_APP_HELPER_H_

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"

#include <vector>
#include <string>

#include "yaml-cpp/yaml.h"

namespace ns3
{
    class CybertwinAppHelper
    {
    public:

        CybertwinAppHelper();
        ~CybertwinAppHelper();

        void InstallApplications(std::string name, YAML::Node parameters, NodeContainer nodes);

    private:
        //void InstallApp(Ptr<Node> node);
        
    };
} // namespace ns3

#endif // _CYBERTWIN_APP_HELPER_H_