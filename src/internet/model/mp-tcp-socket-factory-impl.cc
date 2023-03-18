#include "mp-tcp-socket-factory-impl.h"

#include "tcp-l4-protocol.h"

#include "ns3/assert.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"

// #include "mp-tcp-socket-base.h"

namespace ns3
{

MpTcpSocketFactoryImpl::MpTcpSocketFactoryImpl()
    : m_mptcp(nullptr)
{
}

MpTcpSocketFactoryImpl::~MpTcpSocketFactoryImpl()
{
    NS_ASSERT(!m_mptcp);
}

void
MpTcpSocketFactoryImpl::SetTcp(Ptr<TcpL4Protocol> mptcp)
{
    m_mptcp = mptcp;
}

Ptr<Socket>
MpTcpSocketFactoryImpl::CreateSocket()
{
    return m_mptcp->CreateSocket();
}

void
MpTcpSocketFactoryImpl::DoDispose()
{
    m_mptcp = nullptr;
    MpTcpSocketFactory::DoDispose();
}

} // namespace ns3
