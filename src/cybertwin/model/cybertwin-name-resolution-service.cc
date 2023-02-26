#include "ns3/log.h"
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

NameResolutionService::NameResolutionService(Address super):
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
    InitNameResolutionService();
}

void
NameResolutionService::StopApplication()
{
    //TODO: rememeber to save database.
    NS_LOG_DEBUG("stop CNRS.");
}


void 
NameResolutionService::InitNameResolutionService()
{
    NS_LOG_DEBUG("Init name resolution service.");
    LoadDatabase();
    InitNameResolutionServer();
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

void
NameResolutionService::InitNameResolutionServer()
{
    NS_LOG_FUNCTION(this);
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

    serviceSocket->SetRecvCallback(MakeCallback(&NameResolutionService::RecvHandler, this));
}

void
NameResolutionService::InitReportUDPSocket()
{
    if (superior.IsInvalid())
    {
        reportSocket = nullptr;
        return;
    }
    if (!reportSocket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        reportSocket = Socket::CreateSocket(GetNode(), tid);
        if (reportSocket->Bind() < 0)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        reportSocket->Connect(
            InetSocketAddress(Ipv4Address::ConvertFrom(superior), NAME_RESOLUTION_SERVICE_PORT)
        );
    }
}

void
NameResolutionService::RecvHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this<<socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom(from)))
    {
        CybertwinCNRSHeader rcvHeader;
        packet->RemoveHeader(rcvHeader);
        Ptr<Packet> rspPacket = Create<Packet>(0);

        switch(rcvHeader.GetMethod())
        {
            case CNRS_QUERY:
                QueryRequestHandler(rcvHeader, rspPacket);
                break;
            case CNRS_INSERT:
                InsertRequestHandler(rcvHeader, rspPacket);
                break;
            default:
                NS_LOG_DEBUG("CNRS: Unknown request.");
        }

        socket->Send(rspPacket);
    }
}

void
NameResolutionService::QueryRequestHandler(CybertwinCNRSHeader &rcvHeader, Ptr<Packet> rspPacket)
{
    NS_LOG_DEBUG("CNRS: handle query request.");
    //TODO: packet check
    CYBERTWINID_t id = rcvHeader.GetCybertwinID();
    CybertwinCNRSHeader rspHeader;
    uint32_t ip;
    uint16_t port;

    if (QueryCybertwinItem(id, ip, port) < 0)
    {
        NS_LOG_DEBUG("CNRS: query cybertwinID doesn't exist.");
        rspHeader.SetMethod(CNRS_RESPONSE_FAIL);
    }else
    {
        rspHeader.SetMethod(CNRS_RESPONSE_OK);
        rspHeader.SetCybertwinID(id);
        rspHeader.SetCybertwinAddr(ip);
        rspHeader.SetCybertwinPort(port);
    }
    rspPacket->AddHeader(rspHeader);
}

int
NameResolutionService::QueryCybertwinItem(CYBERTWINID_t id, uint32_t &ip, uint16_t &port)
{
    NS_LOG_DEBUG("Query Cybertwin Item.");
    CYBERTWIN_INTERFACE_t interface;
    if (itemCache.find(id) == itemCache.end())
    {
        return -1;
    }
    interface = itemCache[id];
    ip = interface.first.Get();
    port = interface.second;

    return 0;
}

void
NameResolutionService::InsertRequestHandler(CybertwinCNRSHeader &rcvHeader, Ptr<Packet> rspPacket)
{
    NS_LOG_DEBUG("CNRS: handle insert request.");
    
    CybertwinCNRSHeader rspHeader;
    CYBERTWINID_t name = rcvHeader.GetCybertwinID();
    uint32_t ip = rcvHeader.GetCybertwinAddr();
    uint16_t port = rcvHeader.GetCybertwinPort();

    // insert new item to database
    InsertNewCybertwinItem(name, ip, port);

    // report new item to superior if exist
    ReportName2Superior(name, ip, port);

    rspHeader.SetMethod(CNRS_RESPONSE_OK);
    rspPacket->AddHeader(rspHeader);
}

void
NameResolutionService::InsertNewCybertwinItem(CYBERTWINID_t id, uint32_t ip, uint16_t port)
{
    NS_LOG_DEBUG("Insert New Cybertwin Item.");
    CYBERTWIN_INTERFACE_t interface = std::make_pair(Ipv4Address(ip), port);
    itemCache[id] = interface;
}

void
NameResolutionService::ReportName2Superior(CYBERTWINID_t id, uint32_t ip, uint16_t port)
{
    NS_LOG_DEBUG("Report new item two superior.");
    if (!superior.IsInvalid()) return;
    if (!reportSocket){
        InitReportUDPSocket();
    }
    if (!reportSocket) return ;

    Ptr<Packet> pack = Create<Packet>(128);
    CybertwinCNRSHeader header;
    header.SetMethod(CNRS_INSERT);
    header.SetCybertwinID(id);
    header.SetCybertwinAddr(ip);
    header.SetCybertwinPort(port);

    pack->AddHeader(header);
    reportSocket->Send(pack);
}
} // namespace ns3

