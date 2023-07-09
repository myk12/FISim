#include "ns3/cybertwin-name-resolution-service.h"

#include "ns3/log.h"
#include "ns3/random-variable-stream.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("NameResolutionService");

TypeId
NameResolutionService::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NameResolutionService")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<NameResolutionService>();
    return tid;
}

NameResolutionService::NameResolutionService()
    : serviceSocket(nullptr),
      clientSocket(nullptr),
      m_port(NAME_RESOLUTION_SERVICE_PORT),
      databaseName("testdb")
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("CNRS: create CNRS.");
}

NameResolutionService::NameResolutionService(Ipv4Address super)
    : serviceSocket(nullptr),
      clientSocket(nullptr),
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
    // TODO: rememeber to save database.
    NS_LOG_DEBUG("stop CNRS.");
}

void
NameResolutionService::SetSuperior(Ipv4Address superior)
{
    this->superior = superior;
}

void
NameResolutionService::DefaultGetInterfaceCallback(CYBERTWINID_t id, CYBERTWIN_INTERFACE_LIST_t ifs)
{
    NS_LOG_DEBUG("CNRS: default get interface callback.");
    NS_LOG_DEBUG("CNRS: id: " << id);
    for (auto it = ifs.begin(); it != ifs.end(); it++)
    {
        NS_LOG_DEBUG("CNRS: interface: " << *it);
    }
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
        NS_LOG_DEBUG("CNRS: Init service UDP socket success.");
    }

    serviceSocket->SetRecvCallback(MakeCallback(&NameResolutionService::ServiceRecvHandler, this));
}

int32_t
NameResolutionService::InitClientUDPSocket()
{
    NS_LOG_DEBUG("InitClientUDPSocket.");
    if (!superior.IsInitialized())
    {
        clientSocket = nullptr;
        return -1;
    }

    if (!clientSocket)
    {
        NS_LOG_DEBUG("Init client UDP socket.");
        int ret = 0;
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        clientSocket = Socket::CreateSocket(GetNode(), tid);
        if (clientSocket->Bind() < 0)
        {
            NS_FATAL_ERROR("Failed to bind socket.");
            return -2;
        }
        ret = clientSocket->Connect(InetSocketAddress(superior, NAME_RESOLUTION_SERVICE_PORT));
        if (ret < 0)
        {
            NS_LOG_DEBUG("Failed to connect superior server.");
            return -3;
        }

        clientSocket->SetRecvCallback(
            MakeCallback(&NameResolutionService::ClientRecvHandler, this));
    }
    NS_LOG_DEBUG("CNRS: Init client UDP socket success.");

    return 0;
}

void
NameResolutionService::ClientRecvHandler(Ptr<Socket> socket)
{
    // We currently don't care if we receive the response.
    NS_LOG_DEBUG("CNRS: received response of report request.");
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        CNRSHeader rcvHeader;
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
NameResolutionService::QueryResponseHandler(bool queryOk, CNRSHeader& rcvHeader)
{
    CYBERTWINID_t id = rcvHeader.GetCuid();
    QUERY_ID_t qId = rcvHeader.GetQueryId();
    CYBERTWIN_INTERFACE_LIST_t interfaces;
    if (queryOk)
    {
        NS_LOG_DEBUG("Get query result: {Cuid: " << id << ", QueryId: " << qId << ", InterfaceNum: " << rcvHeader.GetInterfaceList().size() <<"}");
        interfaces = rcvHeader.GetInterfaceList();
        // insert to local
        itemCache[id] = interfaces;
    }

    if (m_queryClientCache.find(qId) != m_queryClientCache.end())
    {
        // query from other node, response
        NS_LOG_DEBUG("Query from Subnode, response.");
        QueryResponse(m_queryClientCache[qId], id, qId, interfaces);
    }
    else if (m_queryCache.find(qId) != m_queryCache.end())
    {
        // query from local
        NS_LOG_DEBUG("Query from local, callback.");
        m_queryCache[qId](id, interfaces);
        //delete cache
        m_queryCache.erase(qId);
    }
    else
    {
        // unknown query response
    }
}

/**
 * @brief: handle query and insert request
 *
 */
void
NameResolutionService::ServiceRecvHandler(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_LOG_DEBUG("CNRS: Handle CNRS request.");
    Ptr<Packet> packet;
    Address from;
    Address localAddress;

    while ((packet = socket->RecvFrom(from)))
    {
        NS_LOG_DEBUG("query from :" << InetSocketAddress::ConvertFrom(from).GetIpv4());
        CNRSHeader rcvHeader;
        packet->RemoveHeader(rcvHeader);
        Ptr<Packet> rspPacket = Create<Packet>(DEFAULT_PAYLOAD_LEN);

        switch (rcvHeader.GetMethod())
        {
        case CNRS_QUERY:
            NS_LOG_DEBUG("CNRS: query request.");
            ProcessQuery(rcvHeader, socket, from);
            break;
        case CNRS_INSERT:
            NS_LOG_DEBUG("CNRS: insert request.");
            ProcessInsert(rcvHeader, socket);
            break;
        default:
            NS_LOG_DEBUG("CNRS: Unknown request.");
        }
    }
}

void
NameResolutionService::ProcessQuery(CNRSHeader& rcvHeader, Ptr<Socket> socket, Address &from)
{
    NS_LOG_FUNCTION(this << rcvHeader);
    // TODO: packet check
    CYBERTWINID_t id = rcvHeader.GetCuid();
    QUERY_ID_t qId = rcvHeader.GetQueryId();

    if (itemCache.find(id) != itemCache.end())
    { // case1: query cybertwinID locally hit
        NS_LOG_DEBUG("CNRS: query cybertwinID hit.");
        InformCNRSResult(id, qId, socket, from);
    }
    else
    { // case2: query cybertwinID locally miss
        NS_LOG_DEBUG("CNRS: query cybertwinID miss. query superior.");
        m_queryClientCache[qId] = std::make_pair(socket, from);
        m_queryCache[qId] = MakeCallback(&NameResolutionService::QueryResponseCallback, this);

        if (QuerySuperior(id, qId) < 0)
        {
            // query fail, response to client
            NS_LOG_DEBUG("CNRS: query cybertwinID miss, query superior fail.");
            QuerySuperiorFail(id, qId);
        }
    }
}

void
NameResolutionService::QueryResponseCallback(CYBERTWINID_t id,
                                             QUERY_ID_t qId,
                                             CYBERTWIN_INTERFACE_LIST_t interfaces)
{
    NS_LOG_FUNCTION(this << id << qId);
    NS_LOG_DEBUG("CNRS: query response callback.");
    // TODO: response to client
}

void
NameResolutionService::QueryResponse(PeerInfo_t peerInfo,
                                     CYBERTWINID_t id,
                                     QUERY_ID_t qid,
                                     CYBERTWIN_INTERFACE_LIST_t ifs)
{
    NS_LOG_FUNCTION(this << id << qid);
    NS_LOG_DEBUG("CNRS: query response to client(subnode).");
    Ptr<Packet> rspPacket = Create<Packet>(DEFAULT_PAYLOAD_LEN);
    CNRSHeader rspHeader;

    rspHeader.SetCuid(id);
    rspHeader.SetQueryId(qid);
    if (ifs.size() == 0)
    {
        rspHeader.SetMethod(CNRS_QUERY_FAIL);
    }
    else
    {
        rspHeader.SetMethod(CNRS_QUERY_OK);
        rspHeader.SetInterfaceList(ifs);
    }

    rspPacket->AddHeader(rspHeader);
    peerInfo.first->SendTo(rspPacket, 0, peerInfo.second);

    //delete record
    m_queryClientCache.erase(qid);
}

void
NameResolutionService::InformCNRSResult(CYBERTWINID_t id, QUERY_ID_t qId, Ptr<Socket> socket, Address &from)
{
    NS_LOG_FUNCTION(this << id);
    NS_LOG_DEBUG("Query response OK.");
    Ptr<Packet> rspPacket = Create<Packet>();
    CNRSHeader rspHeader;
    CYBERTWIN_INTERFACE_LIST_t interfaces = itemCache[id];

    rspHeader.SetMethod(CNRS_QUERY_OK);
    rspHeader.SetCuid(id);
    rspHeader.SetQueryId(qId);
    rspHeader.SetInterfaceList(interfaces);
    rspPacket->AddHeader(rspHeader);

    NS_LOG_DEBUG("send response to client.");
    socket->SendTo(rspPacket, 0, from);
}

int32_t
NameResolutionService::QuerySuperior(CYBERTWINID_t id, QUERY_ID_t qId)
{
    NS_LOG_FUNCTION(this << id);
    // case1: no superior (root)
    if (!superior.IsInitialized())
    {
        NS_LOG_DEBUG("CNRS: no superior.");
        QuerySuperiorFail(id, qId);
        return -1;
    }

    // case2: has superior
    NS_LOG_DEBUG("CNRS: query superior.");
    Ptr<Packet> packet = Create<Packet>();

    CNRSHeader header;
    header.SetMethod(CNRS_QUERY);
    header.SetCuid(id);
    header.SetQueryId(qId);
    packet->AddHeader(header);

    if (clientSocket == nullptr)
    {
        InitClientUDPSocket();
    }

    clientSocket->Send(packet);
    return 0;
}

void
NameResolutionService::QuerySuperiorFail(CYBERTWINID_t id, QUERY_ID_t qId)
{
    NS_LOG_FUNCTION(this << qId);
    if (m_queryCache.find(qId) == m_queryCache.end())
    {
        NS_LOG_DEBUG("CNRS: query cache not found.");
        return;
    }
    CYBERTWIN_INTERFACE_LIST_t itfs;
    m_queryCache[qId](id, itfs);
}

void
NameResolutionService::ProcessInsert(CNRSHeader& rcvHeader, Ptr<Socket> socket)
{
    NS_LOG_DEBUG(this << "CNRS: handle insert request.");

    Ptr<Packet> rspPacket = Create<Packet>();
    CNRSHeader rspHeader;
    CYBERTWINID_t name = rcvHeader.GetCuid();
    CYBERTWIN_INTERFACE_LIST_t interface_list = rcvHeader.GetInterfaceList();

    // insert new item to database
    if (InsertCybertwinInterfaceName(name, interface_list) < 0)
    {
        NS_LOG_DEBUG("CNRS: insert cybertwinID failed.");
        rspHeader.SetMethod(CNRS_INSERT_FAIL);
    }
    else
    {
        rspHeader.SetMethod(CNRS_INSERT_OK);
    }

    rspPacket->AddHeader(rspHeader);
    socket->Send(rspPacket);
}

void
NameResolutionService::ReportName2Superior(CYBERTWINID_t id, CYBERTWIN_INTERFACE_LIST_t interfaces)
{
    NS_LOG_DEBUG("Report new item two superior.");
    if (InitClientUDPSocket() < 0)
    {
        NS_LOG_DEBUG("CNRS: Invaild superior.");
        return;
    }

    Ptr<Packet> pack = Create<Packet>();
    CNRSHeader header;
    header.SetMethod(CNRS_INSERT);
    header.SetCuid(id);
    header.SetInterfaceList(interfaces);
    // header.Print(std::cout);

    pack->AddHeader(header);
    NS_ASSERT_MSG(clientSocket != nullptr, "clientSocket is nullptr.");
    clientSocket->Send(pack);
}

/**
 * @brief get cybertwin interface by name call by application
 *
 * @param name  cybertwin name
 * @param callback  callback when get result
 *
 * @return int32_t 0: success, -1: fail
 */
int32_t
NameResolutionService::GetCybertwinInterfaceByName(
    CYBERTWINID_t name,
    Callback<void, CYBERTWINID_t, CYBERTWIN_INTERFACE_LIST_t> callback)
{
    NS_LOG_DEBUG("Resolve Cybertwin name.");
    if (itemCache.find(name) != itemCache.end())
    {
        // find in cache
        NS_LOG_DEBUG(this << "CNRS: find in local.");
        callback(name, itemCache[name]);
    }
    else
    {
        NS_LOG_DEBUG(this << "CNRS: not find in local. Query from superior.");
        // query from superior
        QUERY_ID_t qId = GetQueryID();
        m_queryCache[qId] = callback;

        QuerySuperior(name, qId);
    }

    return 0;
}

/**
 * @brief insert new item to database and report to superior
 */
int32_t
NameResolutionService::InsertCybertwinInterfaceName(CYBERTWINID_t name,
                                                    CYBERTWIN_INTERFACE_LIST_t& interfaces)
{
    NS_LOG_DEBUG("CNRS: Insert Cybertwin Interface name.");
    NS_LOG_DEBUG("CNRS: name: " << name);
    NS_LOG_DEBUG("CNRS: interfaces: ");
    for (uint32_t i = 0; i < interfaces.size(); i++)
    {
        NS_LOG_DEBUG("CNRS: " << interfaces[i]);
    }
    // insert to cache
    // to prevent circle, check if already exist
    if (itemCache.find(name) != itemCache.end() && itemCache[name] == interfaces)
    {
        NS_LOG_DEBUG("CNRS: item already exist.");
        return 0;
    }

    itemCache[name] = interfaces;
    // report to superior
    Simulator::Schedule(TimeStep(1),
                        &NameResolutionService::ReportName2Superior,
                        this,
                        name,
                        interfaces);
    return 0;
}

QUERY_ID_t
NameResolutionService::GetQueryID()
{
    QUERY_ID_t qId = 0;

    if (m_rand == nullptr)
    {
        m_rand = CreateObject<UniformRandomVariable>();
    }

    do
    {
        qId = m_rand->GetInteger(0, 0xffffffff);
    } while (m_queryCache.find(qId) != m_queryCache.end());

    return qId;
}

} // namespace ns3