#ifndef MP_TCP_SOCKET_FACTORY_IMPL_H
#define MP_TCP_SOCKET_FACTORY_IMPL_H

#include "mp-tcp-socket-factory.h"

namespace ns3
{

class TcpL4Protocol;

/**
 * \ingroup socket
 * \ingroup tcp
 *
 * \brief socket factory implementation for Multipath TCP
 */
class MpTcpSocketFactoryImpl : public MpTcpSocketFactory
{
  public:
    MpTcpSocketFactoryImpl();
    ~MpTcpSocketFactoryImpl() override;

    void SetTcp(Ptr<TcpL4Protocol>);
    Ptr<Socket> CreateSocket() override;

  protected:
    void DoDispose() override;

  private:
    Ptr<TcpL4Protocol> m_mptcp;
};

} // namespace ns3

#endif /* MP_TCP_SOCKET_FACTORY_IMPL_H */
