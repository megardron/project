/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "peer-to-peer.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/object-vector.h"
#include <string>
#include "ns3/seq-ts-header.h"
#include <cstdlib>
#include <cstdio>
#include <list>
#include <iterator>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P2PClient");


int sent = 0;
//sent = sent++;

  std::map<std::string, std::vector<Address>> peers;

NS_OBJECT_ENSURE_REGISTERED (P2PClient);

TypeId
P2PClient::GetTypeId (void)
{
 // messages.Add("hello")
  
  static TypeId tid = TypeId ("ns3::P2PClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<P2PClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&P2PClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
   .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (2.0)),
                   MakeTimeAccessor (&P2PClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&P2PClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&P2PClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&P2PClient::m_size),
                   MakeUintegerChecker<uint32_t> (12,1500));
   /* .AddAttribute ("Packets",
                   "The packets",
                   ObjectVectorValue(),
                   MakeObjectVectorAccessor (&P2PClient::m_packets),
                   MakeObjectVectorChecker<Packet> ()); */
  return tid;
}

P2PClient::P2PClient ()
  : m_lossCounter (0)
{
  NS_LOG_INFO("===============???");
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_received = 0;
  m_socket = 0;
  
  m_packets = std::vector<std::string>(); 
  m_sendEvent = EventId ();
}

P2PClient::P2PClient (std::vector<std::string> packets)
  : m_lossCounter (0)
{
   m_packets = packets;
}

P2PClient::~P2PClient ()
{
  NS_LOG_FUNCTION (this);
}

void
P2PClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
P2PClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
P2PClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
P2PClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      //     InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }
  m_socket->SetRecvCallback (MakeCallback (&P2PClient::HandleRead, this));
  //  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_socket->SetAllowBroadcast (true);
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &P2PClient::Send, this);
}

void
P2PClient::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}

void P2PClient::SetMessages(std::vector<std::string> messages) {
   NS_LOG_FUNCTION(this);
   m_packets = messages;
}

void
P2PClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  uint8_t start[12] = {0x00, 0x00, 0x04, 0x17, 0x27, 0x10, 0x19, 0x80, 0x00, 0x00, 0x00, 0x01};

  NS_LOG_INFO(start);
  std::string s = m_packets[m_sent];
  uint8_t* send = new uint8_t[m_size+12];
  std::fill(send, send+m_size+12, 0x00);
  std::copy (start, start+12, send);
  std::copy (s.c_str(), s.c_str()+s.size(), send+12);
  send[95]=1;
  Ptr<Packet> p = Create<Packet> (send, m_size-(8+4)); // 8+4 : the size of the seqTs header
  uint8_t *buffer = new uint8_t[m_size];
  p->CopyData(buffer, m_size);
  NS_LOG_INFO(buffer[11]);
  p->AddHeader (seqTs);

  std::stringstream peerAddressStringStream;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
    }

  if ((m_socket->Send (p)) >= 0)
    {p->CopyData(buffer, m_size);
      ++m_sent;
      NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to "
                                    << peerAddressStringStream.str () << " Uid: "
                                    << p->GetUid () << " Time: "
                                    << (Simulator::Now ()).GetSeconds ());

    }
  else
    {
      NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                          << peerAddressStringStream.str ());
    }

  if (m_sent < m_count)
    {
      m_sendEvent = Simulator::Schedule (m_interval, &P2PClient::Send, this);
    }
}


void P2PClient::UpdatePeers(std::string received) {
    NS_LOG_FUNCTION(this);
    std::istringstream iss(received);
    std::vector<std::string> parts(( std::istream_iterator<std::string>(iss)),
				   std::istream_iterator<std::string>());
    std::vector<Address> a;
    std::vector<Address>::iterator it;
    //it = a.begin();
    for(uint i = 1; i<parts.size()-1; i=i+2) {
      it = a.end();
      Address A = InetSocketAddress(parts.at(i).c_str(), std::stoi(parts.at(i+1)));
      a.insert(it, A);
    }
    peers.insert(std::pair<std::string, std::vector<Address>>(parts.at(0), a));
    NS_LOG_INFO(a.size());
  }

  
void P2PClient::HandleRead (Ptr<Socket> socket) {
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    { if (packet->GetSize () > 0)
        {
          SeqTsHeader seqTs;
          packet->RemoveHeader (seqTs);
          uint32_t currentSequenceNumber = seqTs.GetSeq ();
           int size = packet->GetSize();
          uint8_t *buffer = new uint8_t[size];
          packet->CopyData(buffer, size);
          NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                          " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                           " Sequence Number: " << currentSequenceNumber <<
                           " Uid: " << packet->GetUid () <<
                           " Packet: " << buffer <<
                           " TXtime: " << seqTs.GetTs () <<
                           " RXtime: " << Simulator::Now () <<
                           " Delay: " << Simulator::Now () - seqTs.GetTs ());
          
          // m_lossCounter.NotifyReceived (currentSequenceNumber);
          //look into why we'd have it and why it doesn't work :(
          m_received++;
          UpdatePeers(std::string((char*) buffer+1));
        } else {
                NS_LOG_INFO("what the fuck");
      }
    }
 }
}
