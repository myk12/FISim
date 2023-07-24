#ifndef DOWNLOAD_CLIENT_H
#define DOWNLOAD_CLIENT_H

#include "ns3/cybertwin-common.h"
#include "ns3/cybertwin-name-resolution-service.h"
#include "ns3/cybertwin-node.h"
#include "ns3/cybertwin-header.h"
#include "ns3/cybertwin-app.h"

namespace ns3
{
class CybertwinEndHost;
class DownloadStream: public Object
{
public:
  static TypeId GetTypeId();

  DownloadStream();
  ~DownloadStream();

  void SetStreamID(uint32_t streamID);
  void SetTargetID(CYBERTWINID_t targetID);
  void SetNode(Ptr<Node> node);
  void SetCUID(CYBERTWINID_t cuid);
  void SetCybertwin(Ipv4Address cybertwinAddress, uint16_t cybertwinPort);
  void SetLogDir(std::string logDir);
  void SetRate(uint8_t rate);

  void Activate();
  //callbacks
  void ConnectionSucceeded(Ptr<Socket>);
  void ConnectionFailed(Ptr<Socket>);
  void ConnectionNormalClosed(Ptr<Socket>);
  void ConnectionErrorClosed(Ptr<Socket>);
  void RecvCallback(Ptr<Socket>);

private:

  Ptr<Node> m_node;
  CYBERTWINID_t m_cuid;

  uint32_t m_streamID;
  CYBERTWINID_t m_targetID;
  Ptr<Socket> m_socket;

  Ipv4Address m_cybertwinAddress;
  uint16_t m_cybertwinPort;

  // statistics
  void DownloadThroughputStatistical();
  Time m_lastTime;
  EventId m_statisticalEvent;
  uint32_t m_intervalBytes;

  std::string m_logDir;
  std::ofstream m_logStream;

  uint8_t m_rate;
};

class DownloadClient : public CybertwinApp
{
public:
  static TypeId GetTypeId();

  DownloadClient();
  virtual ~DownloadClient();
  void AddTargetServer(CYBERTWINID_t cuid, uint8_t rate);

private:
  virtual void StartApplication();
  virtual void StopApplication();

  void StartDownloadStreams();

  Ptr<CybertwinEndHost> m_endHost;
  std::vector<std::pair<CYBERTWINID_t, uint8_t>> m_targetServers;
  std::vector<Ptr<DownloadStream>> m_streams;
};

}

#endif