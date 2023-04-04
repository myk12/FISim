#ifndef CYBERTWIN_NAME_RESOLUTION_SERVICE_H
#define CYBERTWIN_NAME_RESOLUTION_SERVICE_H
#include "ns3/application.h"
#include "ns3/socket.h"
#include "cybertwin-common.h"
#include "cybertwin-packet-header.h"

#include <string>
#include <unordered_map>

namespace ns3
{

class NameResolutionService: public Application
{
    public:
        NameResolutionService();
        NameResolutionService(Ipv4Address superior);
        ~NameResolutionService();
        static TypeId GetTypeId();

        int32_t GetCybertwinInterfaceByName(CYBERTWINID_t name, CYBERTWIN_INTERFACE_LIST_t &interface);
        void InsertCybertwinInterfaceName(CYBERTWINID_t name, CYBERTWIN_INTERFACE_LIST_t &interface);

    private:
        void StartApplication() override;
        void StopApplication() override;

        void LoadDatabase();
        void InitNameResolutionServer();
        int32_t InitReportUDPSocket();

        void ServiceRecvHandler(Ptr<Socket> socket);
        void ReportRecvHandler(Ptr<Socket> socket);

        void QueryRequestHandler(CybertwinCNRSHeader &rcvHeader, Ptr<Packet> rspPacket);
        void QueryResponseHandler(bool status, CybertwinCNRSHeader& rcvHeader);
        void InsertRequestHandler(CybertwinCNRSHeader &rcvHeader, Ptr<Packet> rspPacket);
        void ReportName2Superior(CYBERTWINID_t id, CYBERTWIN_INTERFACE_LIST_t interfaces);

        Ptr<Socket> serviceSocket;
        Ptr<Socket> reportSocket;
        uint16_t m_port;
        Ipv4Address superior;
        std::string databaseName;
        std::unordered_map<CYBERTWINID_t, CYBERTWIN_INTERFACE_LIST_t> itemCache;
};
}//ns3

#endif