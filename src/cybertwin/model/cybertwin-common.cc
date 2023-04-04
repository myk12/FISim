#include "cybertwin-common.h"

namespace ns3
{

void
DoSocketMethod(int (Socket::*Method)(const Address&),
               Ptr<Socket> socket,
               const Address& address,
               uint16_t port)
{
    int ret = -1;
    if (Ipv4Address::IsMatchingType(address))
    {
        const Ipv4Address ipv4 = Ipv4Address::ConvertFrom(address);
        const InetSocketAddress inetSocket = InetSocketAddress(ipv4, port);
        ret = ((*socket).*Method)(inetSocket);
    }
    else if (Ipv6Address::IsMatchingType(address))
    {
        const Ipv6Address ipv6 = Ipv6Address::ConvertFrom(address);
        const Inet6SocketAddress inet6Socket = Inet6SocketAddress(ipv6, port);
        ret = ((*socket).*Method)(inet6Socket);
    }
    if (ret == -1)
    {
        NS_FATAL_ERROR("Failed to execute socket method");
    }
}

uint16_t
DoSocketBind(Ptr<Socket> socket, const Address& address)
{
    uint16_t bindPort = GetBindPort(socket);
    DoSocketMethod(&Socket::Bind, socket, address, bindPort);
    return bindPort;
}

uint16_t
GetBindPort(Ptr<Socket> socket)
{
    int flag = socket->Bind();
    NS_ASSERT_MSG(flag != -1, "Failed to allocate a port");
    Address sockAddr;
    socket->GetSockName(sockAddr);
    uint16_t ret = 0;
    if (InetSocketAddress::IsMatchingType(sockAddr))
    {
        ret = InetSocketAddress::ConvertFrom(sockAddr).GetPort();
    }
    else if (Inet6SocketAddress::IsMatchingType(sockAddr))
    {
        ret = Inet6SocketAddress::ConvertFrom(sockAddr).GetPort();
    }
    return ret;
}

} // namespace ns3