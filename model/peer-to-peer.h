/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef PEER_TO_PEER_H
#define PEER_TO_PEER_H

#include "ns3/application.h"
#include "ns3/udp-client.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/packet-loss-counter.h"
#include "ns3/ipv4.h"
#include "ns3/network-module.h"
namespace ns3 {


class Socket;
class Packet;



/**
 * \ingroup p2pclientserver
 *
 * \brief Extenstion of a Udp client. Sends UDP packet carrying sequence number and time stamp, and a request for a bit of data
 *  in their payloads
 *
 */
class P2PClient : public UdpClient
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  std::list<uint8_t*> messages;

  P2PClient ();

  P2PClient(std::vector<std::string> packets);

  void PrintDataServed(int n);
  void ScheduleEvents(std::vector<std::string> events);
  virtual ~P2PClient ();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address ip, uint16_t port);
  /**
   * \brief set the remote address
   * \param addr remote address
   */
  void SetRemote (Address addr);
  void SetMessages(std::vector<std::string> messages);
  PacketLossCounter m_lossCounter; //!< Lost packet counter

  
protected:
  virtual void DoDispose (void);

private:
  std::string UpdatePeers(std::string received);
  void HandleRead(Ptr<Socket> socket);
  void HandleTcp(Ptr<Socket> socket);
  void HandlePeerClose(Ptr<Socket> socket);
  void HandlePeerError(Ptr<Socket> socket);
  void HandleConnect(Ptr<Socket> socket);
  void HandleConnectError(Ptr<Socket> socket);
  void HandleAccept(Ptr<Socket> socket, const Address& from);
  void SetupTCPConnections(std::string filename);
  void SendPacket(Ptr<Socket> sock, Ptr<Packet> packet);
  void ScheduleTx(Ptr<Socket> sock, Ptr<Packet> packet);
  void ExpireCache(std::string file);
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * \brief Send a packet
   */
  void Send (void);

  uint32_t m_count; //!< Maximum number of packets the application will send
  uint32_t m_mode;
  uint32_t m_sentTime;
  Time m_interval; //!< Packet inter-send time
  uint32_t m_size; //!< Size of the sent packet (including the SeqTsHeader)
  uint32_t m_received;
  uint32_t m_cacheTime;
  ns3::DataRate m_dataRate;
  uint32_t m_packetSize;
  uint32_t m_tcpSent;
  uint32_t m_nPackets;
  uint32_t m_totalRx;
  std::map<std::string, std::vector<Address>> peers;
    std::set<std::string> cache;
  bool m_connected;
  Ipv4Address m_localIpv4;
  std::list<Ptr<Socket> > m_socketList; //!< the accepted sockets
  
  uint32_t m_sent; //!< Counter for sent packets
  Ptr<Socket> m_socket; //!< Socket
  Ptr<Socket> m_socket_tcp;
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port
  EventId m_sendEvent; //!< Event to send the next packet
  std::vector<std::string> m_packets;
  std::vector<std::string> m_data;
  int dataServed;


};
}

#endif /* PEER_TO_PEER_H */

