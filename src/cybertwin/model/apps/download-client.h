#ifndef DOWNLOAD_CLIENT_H
#define DOWNLOAD_CLIENT_H

#include "ns3/cybertwin-common.h"
#include "ns3/cybertwin-name-resolution-service.h"
#include "ns3/cybertwin-node.h"

namespace ns3
{
class DownloadClient : public Application
{
public:
  DownloadClient(uint64_t cuid);
  virtual ~DownloadClient();

private:
  virtual void StartApplication();
  virtual void StopApplication();

  void ResolveCUID();

  uint64_t m_cuid;
  Ptr<NameResolutionService> m_cnrs;
  Address m_serverAddress;
  Ptr<Socket> m_socket;
  Ptr<CybertwinEndHost> m_endHost;
};

}

#endif