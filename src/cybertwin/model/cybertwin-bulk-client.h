#ifndef CYBERTWIN_BULK_CLIENT_H
#define CYBERTWIN_BULK_CLIENT_H

#include "cybertwin-packet-header.h"
#include "cybertwin-common.h"

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
    
    // CybertwinController Relative
    void ControllerConnectSucceededCallback(Ptr<Socket>);
    void ControllerConnectFailedCallback(Ptr<Socket>);
    void RecvFromControllerCallback(Ptr<Socket> socket); 
    void Request2GenerateCybertwin();
    void ConnectCybertwin();

    // Cybertwin Relative
    void CybertwinConnectSucceededCallback(Ptr<Socket> socket);
    void CybertwinConnectFailedCallback(Ptr<Socket> socket);
    void RecvFromCybertwinCallback(Ptr<Socket> socket);

    void RequestNetworkService();

  protected:
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    void Connect();

    Ptr<Socket> controllerSocket;
    Ptr<Socket> cybertwinSocket;

    Address m_localAddr;
    uint16_t m_localPort;
    uint64_t m_localGuid;

    Address m_edgeAddr;
    uint16_t m_edgePort;

    uint64_t m_peerGuid;

    uint32_t m_sendSize;

    CYBERTWINID_t cybertwinID;
    uint16_t cybertwinPort;
};

} // namespace ns3

#endif