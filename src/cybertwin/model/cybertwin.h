#ifndef CYBERTWIN_H
#define CYBERTWIN_H

#include "ns3/core-module.h"
#include "ns3/application.h"
#include "ns3/socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/node.h"
#include "ns3/internet-module.h"
#include "ns3/type-id.h"
#include "ns3/network-module.h"

namespace ns3
{
//class Application;
/* ... */
class CybertwinServer: public Application
{
    public:
        CybertwinServer();
        ~CybertwinServer() override;

        /**
         * @brief Get the Type Id object
         * 
         * @return TypeId 
         */
        static TypeId GetTypeId();

        void Setup();

    private:
        void StartApplication() override;
        void StopApplication() override;
        void RecvHandler(Ptr<Socket>);
        void NewConnectionCreatedCallback(Ptr<Socket> socket, const Address& addr);
        bool ConnectionRequestCallback(Ptr<Socket> socket, const Address& addr);

        Ptr<Socket> m_socket;
        Ptr<Node> node;
        Ipv4Address ipaddr;
        uint16_t port;
};


class CybertwinClient: public Application 
{
    public:
        CybertwinClient();
        ~CybertwinClient() override;

        static TypeId GetTypeId();

        void Setup(Ipv4Address, uint16_t);

    private:
        void StartApplication() override;
        void StopApplication() override;

        Ptr<Socket> m_socket;
        Ptr<Node> node;
        Ipv4Address m_peerAddr;
        uint16_t m_peerPort;
};

}

#endif /* CYBERTWIN_H */
