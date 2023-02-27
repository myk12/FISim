#include "ns3/log.h"
#include "ns3/core-module.h"
#include "cybertwin-name-resolution-service.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("NameResolutionService");

TypeId
NameResolutionService::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NameResolutionService")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<NameResolutionService>();
    return tid;
}

NameResolutionService::NameResolutionService():
    serviceSocket(nullptr),
    reportSocket(nullptr),
    m_port(NAME_RESOLUTION_SERVICE_PORT),
    databaseName("testdb")
{
    NS_LOG_FUNCTION(this);
}

NameResolutionService::NameResolutionService(Ipv4Address super):
    serviceSocket(nullptr),
    reportSocket(nullptr),
    m_port(NAME_RESOLUTION_SERVICE_PORT),
    superior(super),
    databaseName("testdb")
{
}

NameResolutionService::~NameResolutionService()
{
    NS_LOG_FUNCTION(this);
    serviceSocket = nullptr;
}

void
NameResolutionService::StartApplication()
{
    NS_LOG_DEBUG("Init name resolution service.");
    LoadDatabase();
    InitNameResolutionServer();
}

void
NameResolutionService::StopApplication()
{
    //TODO: rememeber to save database.
    NS_LOG_DEBUG("stop CNRS.");
}

void 
NameResolutionService::LoadDatabase()
{
    NS_LOG_DEBUG("loadDatabase.");
    if (databaseName.size() != 0)
    {
        NS_LOG_DEBUG("loading database");
    }
}

/**
 * brief: init name resoltuion service
*/
void
NameResolutionService::InitNameResolutionServer()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("CNRS: Init Name Resolution Service.");
    if (!serviceSocket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        serviceSocket = Socket::CreateSocket(GetNode(), tid);
        InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);

        if (serviceSocket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
    }

    serviceSocket->SetRecvCallback(MakeCallback(&NameResolutionService::ServiceRecvHandler, this));
}

int32_t
NameResolutionService::InitReportUDPSocket()
{
    if (!superior.IsInitialized())
    {
        reportSocket = nullptr;
        return -1;
    }

    if (!reportSocket)
    {
        int ret = 0;
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        reportSocket = Socket::CreateSocket(GetNode(), tid);
        if (reportSocket->Bind() < 0)
        {
            NS_FATAL_ERROR("Failed to bind socket.");
            return -2;
        }
        ret = reportSocket->Connect(InetSocketAddress(superior, NAME_RESOLUTION_SERVICE_PORT));
        if (ret < 0)
        {
            NS_LOG_DEBUG("Failed to connect superior server.");
            return -3;
        }

        reportSocket->SetRecvCallback(MakeCallback(&NameResolutionService::ReportRecvHandler, this));
    }

    return 0;
}

void
NameResolutionService::ReportRecvHandler(Ptr<Socket> socket)
{
    // We currently don't care if we receive the response.
    NS_LOG_DEBUG("CNRS: received response of report request.");
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        CybertwinCNRSHeader rcvHeader;
        packet->RemoveHeader(rcvHeader);
        
        switch (rcvHeader.GetMethod())
        {
        case CNRS_QUERY_OK:
            NS_LOG_DEBUG("CNRS: Get query response OK.");
            QueryResponseHandler(true, rcvHeader);
            break;
        case CNRS_QUERY_FAIL:
            NS_LOG_DEBUG("CNRS: Get query response FAIl.");
            QueryResponseHandler(false, rcvHeader);
            break;
        case CNRS_INSERT_OK:
            NS_LOG_DEBUG("CNRS: Get insert response OK.");
            break;
        case CNRS_INSERT_FAIL:
            NS_LOG_DEBUG("CNRS: Get insert response FAIL.");
            break;
        default:
            NS_LOG_DEBUG("CNRS: Get unknown response.");
            break;
        }
    }
}

void
NameResolutionService::QueryResponseHandler(bool status, CybertwinCNRSHeader& rcvHeader)
{
    if (!status)
    {
        NS_LOG_DEBUG(this<<"CNRS: query cyebrtwin ID failed!!!");
    }

    int method = rcvHeader.GetMethod();

    if (method == CNRS_QUERY_OK)
    {
        CYBERTWINID_t id    = rcvHeader.GetCybertwinID();
        uint32_t ip         = rcvHeader.GetCybertwinAddr();
        uint16_t port       = rcvHeader.GetCybertwinPort();
            
        // insert to cache
        Ipv4Address addr(ip);
        itemCache[id] = std::make_pair(addr, port);
        NS_LOG_DEBUG(this<<"CNRS: Query Cybertwin "<<id<<std::endl<<"ip: "<<ip<<std::endl<<"port: "<<port);
    }else if (method == CNRS_QUERY_FAIL)
    {
        NS_LOG_DEBUG(this<<"CNRS: query fialed.");
    }
}

void
NameResolutionService::ServiceRecvHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this<<socket);
    NS_LOG_DEBUG("CNRS: Handle CNRS request.");
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom(from)))
    {
        CybertwinCNRSHeader rcvHeader;
        packet->RemoveHeader(rcvHeader);
        Ptr<Packet> rspPacket = Create<Packet>(DEFAULT_PAYLOAD_LEN);

        switch(rcvHeader.GetMethod())
        {
            case CNRS_QUERY:
                NS_LOG_DEBUG("CNRS: query request.");
                QueryRequestHandler(rcvHeader, rspPacket);
                break;
            case CNRS_INSERT:
                NS_LOG_DEBUG("CNRS: insert request.");
                InsertRequestHandler(rcvHeader, rspPacket);
                break;
            default:
                NS_LOG_DEBUG("CNRS: Unknown request.");
        }

        NS_LOG_DEBUG(this<<"CNRS: Response Query sendout.");
        socket->SendTo(rspPacket, 0, from);
    }
}

void
NameResolutionService::QueryRequestHandler(CybertwinCNRSHeader &rcvHeader, Ptr<Packet> rspPacket)
{
    NS_LOG_DEBUG(this<<"CNRS: handle query request.");
    //TODO: packet check
    CYBERTWINID_t id = rcvHeader.GetCybertwinID();
    CybertwinCNRSHeader rspHeader;
    CYBERTWIN_INTERFACE_t interface;

    if (GetCybertwinInterfaceByName(id, interface) < 0)
    {
        NS_LOG_DEBUG("CNRS: query cybertwinID doesn't exist.");
        rspHeader.SetMethod(CNRS_QUERY_FAIL);
    }else
    {
        NS_LOG_DEBUG("CNRS: query cybertwinID found.");
        rspHeader.SetMethod(CNRS_QUERY_OK);
        rspHeader.SetCybertwinID(id);
        rspHeader.SetCybertwinAddr(interface.first.Get());
        rspHeader.SetCybertwinPort(interface.second);
    }
    rspPacket->AddHeader(rspHeader);
}

void
NameResolutionService::InsertRequestHandler(CybertwinCNRSHeader &rcvHeader, Ptr<Packet> rspPacket)
{
    NS_LOG_DEBUG(this<<"CNRS: handle insert request.");
    
    CybertwinCNRSHeader rspHeader;
    CYBERTWINID_t name = rcvHeader.GetCybertwinID();
    CYBERTWIN_INTERFACE_t interface;
    interface.first = Ipv4Address(rcvHeader.GetCybertwinAddr());
    interface.second = rcvHeader.GetCybertwinPort();

    // insert new item to database
    InsertCybertwinInterfaceName(name, interface);

    rspHeader.SetMethod(CNRS_INSERT_OK);
    rspPacket->AddHeader(rspHeader);
}

void
NameResolutionService::ReportName2Superior(CYBERTWINID_t id, uint32_t ip, uint16_t port)
{
    NS_LOG_DEBUG("Report new item two superior.");
    if (InitReportUDPSocket() < 0)
    {
        NS_LOG_DEBUG("CNRS: Invaild superior.");
        return;
    }

    Ptr<Packet> pack = Create<Packet>(DEFAULT_PAYLOAD_LEN);
    CybertwinCNRSHeader header;
    header.SetMethod(CNRS_INSERT);
    header.SetCybertwinID(id);
    header.SetCybertwinAddr(ip);
    header.SetCybertwinPort(port);

    pack->AddHeader(header);
    reportSocket->Send(pack);
}

int32_t
NameResolutionService::GetCybertwinInterfaceByName(CYBERTWINID_t name, CYBERTWIN_INTERFACE_t &interface)
{
    NS_LOG_DEBUG("Resolve Cybertwin name.");
    static int maxTry = 16;
    // find in cache

    if (itemCache.find(name) != itemCache.end())
    {
        NS_LOG_DEBUG(this<<"CNRS: find in local.");
        interface = itemCache[name];
        return 1;
    }

    if (maxTry > 0)
    {
        maxTry--;

        // send query to superior
        if (InitReportUDPSocket() < 0)
        {
            NS_LOG_DEBUG("CNRS: Invaild superior.");
            return -1;
        }

        Ptr<Packet> pack = Create<Packet>(DEFAULT_PAYLOAD_LEN);
        CybertwinCNRSHeader header;
        header.SetMethod(CNRS_QUERY);
        header.SetCybertwinID(name);

        pack->AddHeader(header);
        reportSocket->Send(pack);

        Simulator::Schedule(Seconds(1.),
                        &NameResolutionService::GetCybertwinInterfaceByName,
                        this,
                        name,
                        interface);
    }

    return -1;
}

void
NameResolutionService::InsertCybertwinInterfaceName(CYBERTWINID_t name, CYBERTWIN_INTERFACE_t &interface)
{
    NS_LOG_DEBUG("CNRS: Insert Cybertwin Interface name.");
    // insert to cache
    //TODO: insert to databse
    itemCache[name] = interface;

    // report to superior
    if (InitReportUDPSocket() < 0)
    {
        NS_LOG_DEBUG("CNRS: Invaild superior.");
        return ;
    }

    // report new item to superior if exist
    ReportName2Superior(name, interface.first.Get(), interface.second);
}


} // namespace ns3

