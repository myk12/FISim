#ifndef CYBERTWIN_EDGE_SERVER_H
#define CYBERTWIN_EDGE_SERVER_H

#include "cybertwin-packet-header.h"
#include "cybertwin.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ptr.h"
#include "ns3/seq-ts-size-header.h"
#include "ns3/traced-callback.h"

#include <string>
#include <unordered_map>

namespace ns3
{
class Address;
class Socket;
class Packet;

class AddressHash
{
    size_t operator()(const Address& x) const
    {
        if (InetSocketAddress::IsMatchingType(x))
        {
            InetSocketAddress a = InetSocketAddress::ConvertFrom(x);
            return Ipv4AddressHash()(a.GetIpv4());
        }
        else if (Inet6SocketAddress::IsMatchingType(x))
        {
            Inet6SocketAddress a = Inet6SocketAddress::ConvertFrom(x);
            return Ipv6AddressHash()(a.GetIpv6());
        }

        NS_ABORT_MSG("PacketSink: unexpected address type, neither IPv4 nor IPv6");
        return 0; // silence the warnings.
    }
};

// Global GUID Table (temporary)
std::unordered_map<uint64_t, Address> GuidTable;

class CybertwinItem : public SimpleRefCount<CybertwinItem>
{
  public:
    CybertwinItem(uint64_t, const Address&, Ptr<Socket>);
    ~CybertwinItem();

    // void SendTo(uint64_t, Ptr<Packet>, bool);
    // void RecvFrom(uint64_t, Ptr<Packet>, bool);
    void SendToClient(Ptr<Packet>);

  private:
    uint64_t m_guid;
    Address m_clientAddr;
    Ptr<Socket> m_socket;
};

class CybertwinControlTable : public SimpleRefCount<CybertwinControlTable>
{
  public:
    CybertwinControlTable();

    Ptr<CybertwinItem> Get(uint64_t);
    Ptr<CybertwinItem> Connect(uint64_t, const Address&, Ptr<Socket>);
    void Disconnect(uint64_t);

    void DoDispose();

  private:
    std::unordered_map<uint64_t, Ptr<CybertwinItem>> m_table;
};

class CybertwinController : public Application
{
  public:
    static TypeId GetTypeId();
    CybertwinController();

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    bool ConnectionRequestCallback(Ptr<Socket> socket, const Address& address);
    void NewConnectionCreatedCallback(Ptr<Socket> socket, const Address& address);

    void NormalCloseCallback(Ptr<Socket> socket);
    void ErrorCloseCallback(Ptr<Socket> socket);

    void ReceivedDataCallback(Ptr<Socket> socket);
    void ReceivedDataCallback2(Ptr<Socket> socket);

    uint32_t BornCybertwin(CybertwinControllerHeader header);

    Ptr<Socket> m_listenSocket;
    Ptr<CybertwinControlTable> m_controlTable;

    struct StreamState : public SimpleRefCount<StreamState>
    {
        uint32_t bytesToReceive{0};
        uint64_t srcGuid{0};
        uint64_t dstGuid{0};
        uint16_t action{0};
        bool isClientSocket{false};

        void update(CybertwinPacketHeader& header)
        {
            if (srcGuid == 0 && dstGuid == 0 && action == 0 && bytesToReceive == 0)
            {
                action = header.GetCmd();
                if (action == 0)
                {
                    isClientSocket = true;
                }
            }
            bytesToReceive = header.GetSize();
            srcGuid = header.GetSrc();
            dstGuid = header.GetDst();
        }
    };

    std::unordered_map<Ptr<Socket>, Ptr<StreamState>> m_streamBuffer;
    Address m_localAddr;
    uint64_t m_localPort;
    uint16_t localPortCounter;
    uint16_t globalPortCounter;
    std::unordered_map<CYBERTWINID_t, Ptr<Cybertwin>> CybertwinMapTable;
};

} // namespace ns3

#endif