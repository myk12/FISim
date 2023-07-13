#ifndef DOWNLOAD_SERVER_H
#define DOWNLOAD_SERVER_H

#include "ns3/cybertwin-common.h"
#include "ns3/cybertwin-node.h"

namespace ns3
{
class DownloadServer: public Application
{
  public:
    DownloadServer();
    DownloadServer(CYBERTWINID_t, CYBERTWIN_INTERFACE_LIST_t);
    ~DownloadServer();

    static TypeId GetTypeId();

  private:
    void StartApplication();
    void StopApplication();

    void Init();
    // callbacks
    bool ConnRequestCallback(Ptr<Socket>, const Address&);
    void ConnCreatedCallback(Ptr<Socket>, const Address&);
    void NormalCloseCallback(Ptr<Socket>);
    void ErrorCloseCallback(Ptr<Socket>);

    void RecvCallback(Ptr<Socket>);
    void BulkSend(Ptr<Socket>);

  private:
    CYBERTWINID_t m_cybertwinID;
    CYBERTWIN_INTERFACE_LIST_t m_interfaces;
#if MDTP_ENABLED
    // Cybertwin Connections
    CybertwinDataTransferServer* m_dtServer;
#else
    Ptr<Socket> m_dtServer;
#endif
    std::unordered_map<Ptr<Socket>, uint64_t> m_sendBytes;

    uint32_t m_maxBytes;
};
} // namespace ns3

#endif // DOWNLOAD_SERVER_H