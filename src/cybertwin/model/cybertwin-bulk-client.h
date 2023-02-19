#ifndef CYBERTWIN_BULK_CLIENT_H
#define CYBERTWIN_BULK_CLIENT_H

#include "cybertwin-packet-header.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include <stdlib.h>
#include <time.h>

namespace ns3
{

class Socket;
class Packet;

class CybertwinBulkClient : public Application
{
  public:
    CybertwinBulkClient();
    ~CybertwinBulkClient();

    static TypeId GetTypeId();

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    void Connect();
    void ConnectionSucceeded(Ptr<Socket>);
    void ConnectionFailed(Ptr<Socket>);

    void ReceivedData(Ptr<Socket>);

    void sendMethod();

    Ptr<Socket> m_socket;

    Address m_localAddr;
    uint16_t m_localPort;
    uint64_t m_localGuid;

    Address m_edgeAddr;
    uint16_t m_edgePort;

    uint64_t m_peerGuid;

    uint32_t m_sendSize;
};

} // namespace ns3

#endif