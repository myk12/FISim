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
        NameResolutionService(Address superior);
        ~NameResolutionService();
        static TypeId GetTypeId();

        void InitNameResolutionService();

    private:
        void StartApplication() override;
        void StopApplication() override;

        void LoadDatabase();
        void InitNameResolutionServer();
        void InitReportUDPSocket();

        void RecvHandler(Ptr<Socket> socket);

        void QueryRequestHandler(CybertwinCNRSHeader &rcvHeader, Ptr<Packet> rspPacket);
        int QueryCybertwinItem(CYBERTWINID_t id, uint32_t &ip, uint16_t &port);
        void InsertRequestHandler(CybertwinCNRSHeader &rcvHeader, Ptr<Packet> rspPacket);
        void InsertNewCybertwinItem(CYBERTWINID_t id, uint32_t ip, uint16_t port);

        void ReportName2Superior(CYBERTWINID_t id, uint32_t ip, uint16_t port);

        Ptr<Socket> serviceSocket;
        Ptr<Socket> reportSocket;
        uint16_t m_port;
        Address superior;
        std::string databaseName;
        std::unordered_map<CYBERTWINID_t, CYBERTWIN_INTERFACE_t> itemCache;
};
}//ns3

#endif