#ifndef MPTCP_SOCKET_FACTORY_H
#define MPTCP_SOCKET_FACTORY_H

#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-factory.h"

namespace ns3
{
class TcpSocketFactory;
class Socket;

class MpTcpSocketFactory : public SocketFactory
{
  public:
    static TypeId GetTypeId();
};

} // namespace ns3

#endif /* MPTCP_SOCKET_FACTIRY_H */
