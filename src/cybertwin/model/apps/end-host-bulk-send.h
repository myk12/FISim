#ifndef END_HOST_BULK_SEND_H
#define END_HOST_BULK_SEND_H

#include "ns3/cybertwin-common.h"
#include "ns3/cybertwin-node.h"

namespace ns3
{

class EndHostBulkSend : public Application
{
  public:
    EndHostBulkSend();
    ~EndHostBulkSend();

    static TypeId GetTypeId();

  private:
    void StartApplication();
    void StopApplication();

    void ConnectCybertwin();

    // callbacks
    void ConnectionSucceeded(Ptr<Socket>);
    void ConnectionFailed(Ptr<Socket>);
    void ConnectionNormalClosed(Ptr<Socket>);
    void ConnectionErrorClosed(Ptr<Socket>);

    void RecvData(Ptr<Socket>);
    void SendData();

  private:
    //private member variables
    Ipv4Address m_cybertwinAddr;
    uint16_t m_cybertwinPort;

    Ptr<Socket> m_socket;

    uint32_t m_totalBytes;
    uint32_t m_sentBytes;
};
    
} // namespace ns

#endif // END_HOST_BULK_SEND_H