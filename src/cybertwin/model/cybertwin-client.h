#ifndef CYBERTWIN_CLIENT_H
#define CYBERTWIN_CLIENT_H

#include "cybertwin-common.h"
#include "cybertwin-packet-header.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"

namespace ns3
{

class Socket;
class Packet;

class CybertwinClient : public Application
{
  public:
    CybertwinClient();

    static TypeId GetTypeId();

  protected:
    void DoDispose() override;

    virtual void RecvPacket(Ptr<Socket>);

    // socket connected to the cybertwin
    Ptr<Socket> m_socket;
    CYBERTWINID_t m_localCuid;
    CYBERTWINID_t m_peerCuid;
    Time m_startTime;
};

class CybertwinConnClient : public CybertwinClient
{
  public:
    CybertwinConnClient();
    ~CybertwinConnClient();

    static TypeId GetTypeId();

  protected:
    void DoDispose() override;

  private:
    // CybertwinController
    void ControllerConnectSucceededCallback(Ptr<Socket>);
    void ControllerConnectFailedCallback(Ptr<Socket>);
    void RecvFromControllerCallback(Ptr<Socket>);

    // Cybertwin
    void CybertwinConnectSucceededCallback(Ptr<Socket>);
    void CybertwinConnectFailedCallback(Ptr<Socket>);
    // Need these callbacks because the edge server might stop before the client
    void CybertwinCloseSucceededCallback(Ptr<Socket>);
    void CybertwinCloseFailedCallback(Ptr<Socket>);

    // Basic Functions
    void GenerateCybertwin();
    void ConnectCybertwin();
    void DisconnectCybertwin();

    void StartApplication() override;
    void StopApplication() override;

    Address m_localAddr;
    uint16_t m_localPort;

    Address m_edgeAddr;
    uint16_t m_edgePort;

    Ptr<Socket> m_controllerSocket;
    uint16_t m_cybertwinPort;
};

class CybertwinBulkClient : public CybertwinClient
{
  public:
    CybertwinBulkClient();
    ~CybertwinBulkClient();

    static TypeId GetTypeId();

  protected:
    // void DoInitialize() override;
    void DoDispose() override;

  private:
    void StartApplication() override;
    void StopApplication() override;

    void SendData();

    uint64_t m_maxBytes;
    uint64_t m_sentBytes;
    uint32_t m_packetSize;

    Ptr<Packet> m_unsentPacket;
};

} // namespace ns3

#endif