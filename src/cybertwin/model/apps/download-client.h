#ifndef DOWNLOAD_CLIENT_H
#define DOWNLOAD_CLIENT_H

#include "ns3/cybertwin-common.h"
#include "ns3/cybertwin-name-resolution-service.h"
#include "ns3/cybertwin-node.h"
#include "ns3/cybertwin-header.h"

namespace ns3
{
class CybertwinEndHost;
class DownloadStream
{
public:
  DownloadStream();
  ~DownloadStream();

  void SetStreamID(uint32_t streamID);
  void SetTargetID(CYBERTWINID_t targetID);
  void SetNode(Ptr<Node> node);
  void SetCUID(CYBERTWINID_t cuid);
  void SetCybertwin(Ipv4Address cybertwinAddress, uint16_t cybertwinPort);

  void Activate();
  //callbacks
  void ConnectionSucceeded(Ptr<Socket>);
  void ConnectionFailed(Ptr<Socket>);
  void ConnectionNormalClosed(Ptr<Socket>);
  void ConnectionErrorClosed(Ptr<Socket>);
  void RecvCallback(Ptr<Socket>);

private:
  //function
  void DownloadThroughputStatistical();
  Time m_lastTime;
  uint32_t m_intervalBytes;
  EventId m_statisticalEvent;
private:

  Ptr<Node> m_node;
  CYBERTWINID_t m_cuid;

  uint32_t m_streamID;
  CYBERTWINID_t m_targetID;
  Ptr<Socket> m_socket;

  Ipv4Address m_cybertwinAddress;
  uint16_t m_cybertwinPort;
};

class DownloadClient : public Application
{
public:
  static TypeId GetTypeId();

  DownloadClient();
  DownloadClient(std::vector<CYBERTWINID_t> cuidList);
  virtual ~DownloadClient();

  void AddCUID(CYBERTWINID_t cuid);

private:
  virtual void StartApplication();
  virtual void StopApplication();

  void StartDownloadStreams();

  std::vector<CYBERTWINID_t> m_cuidList;
  Ptr<CybertwinEndHost> m_endHost;
  std::vector<DownloadStream> m_streams;
};

}

#endif