/* -*- Mode:C++; c-file-style:"gnu"; sindent-tabs-mode:nil; -*- */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/ipv4.h"
#include "ns3/uinteger.h"
#include "ns3/packet-loss-counter.h"

#include "ns3/seq-ts-header.h"
#include "peer-to-peer-server.h"

#include <iterator>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("P2PServer");

NS_OBJECT_ENSURE_REGISTERED (P2PServer);

  std::map<std::string, std::set<Address>> torrents;
TypeId
P2PServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::P2PServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<P2PServer> ()
    .AddAttribute ("Port",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&P2PServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketWindowSize",
                   "The size of the window used to compute the packet loss. This value should be a multiple of 8.",
                   UintegerValue (32),
                   MakeUintegerAccessor (&P2PServer::GetPacketWindowSize,
                                         &P2PServer::SetPacketWindowSize),
                   MakeUintegerChecker<uint16_t> (8,256))
  ;
  return tid;
}

P2PServer::P2PServer ()
  : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received=0;
  m_sent = 0;
}

P2PServer::~P2PServer ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
P2PServer::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
P2PServer::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
P2PServer::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetLost ();
}

uint64_t
P2PServer::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
}

  void P2PServer::DoDispose (void) {
    NS_LOG_FUNCTION (this);
    Application::DoDispose ();
  }

  Ptr<Packet> P2PServer::CreateReplyPacket(uint8_t* bytes, int size) {
    Ptr<Packet> p = Create<Packet> (bytes, size);
    return p;
  }

  int P2PServer::ParseAction(uint8_t* message) {
     return message[11];
  }

  
  
  void P2PServer::RemoveTorrent(Address from, std::string received) {
    NS_LOG_FUNCTION(this);
    std::istringstream iss(received);
    std::vector<std::string> parts(( std::istream_iterator<std::string>(iss)),
				   std::istream_iterator<std::string>());
    std::set<Address> a;
    std::set<Address>::iterator it;
    std::string filename = parts[0] + " " + parts[1];
    if (torrents.count(filename)!=0) {
      a = torrents.at(filename);
      torrents.erase(filename);
      if (a.count(from)!=0) {
	a.erase(from);
      }
      torrents.insert(std::pair<std::string, std::set<Address>>(filename, a));
    }
  }
  
  std::string P2PServer::AddTorrentReturnPeers(Address from, std::string received) {
    NS_LOG_FUNCTION(this);
    std::istringstream iss(received);
    std::vector<std::string> parts(( std::istream_iterator<std::string>(iss)),
				   std::istream_iterator<std::string>());
    std::set<Address> a;
    std::set<Address>::iterator it;
    std::string filename = parts[1] + " " + parts[2];
    if (torrents.count(filename)!=0) {
	a = torrents.at(filename);
        torrents.erase(filename);

    } else {
      it = a.begin();
      Ptr<Ipv4> ipv4 =  this->m_node->GetObject<Ipv4>();
      Address local = (InetSocketAddress) ipv4->GetAddress(1,0).GetLocal();
      a.insert(it, local);
    }
    it = a.begin();
    a.insert(it, from);
    torrents.insert(std::pair<std::string, std::set<Address>>(filename, a));
    //return all addresses
    //limit?
    std::string reply;
    reply += filename;
    reply += " ";
    for (it = a.begin(); it != a.end(); it++) {
      std::ostringstream oss;
      InetSocketAddress::ConvertFrom((*it)).GetIpv4().Print(oss);
      std::stringstream ss;
      ss << InetSocketAddress::ConvertFrom((*it)).GetPort();
      //std::string temp = (std::string) InetSocketAddress::ConvertFrom(a.at(i)).GetIpv4();
      reply += oss.str();
      reply += " ";
      reply += ss.str();
      reply += " ";
    }
    return reply;
  }

void P2PServer::Reply(Address from,Ptr<Packet> pckt) {
  NS_LOG_FUNCTION(this);
        //maybe need a unique id, ot_udp.c - probably necessary for realism
    //also hash pointer?
    //if we are using the unique id need some logic after first connect
    //otherwise....
  int size = pckt->GetSize();
  uint8_t *buffer = new uint8_t[size];
        std::fill(buffer, buffer+size, 0x00);
  pckt->CopyData(buffer, size);
  //std::copy(buffer+12, buffer+size, buffer);
   int action = ParseAction(buffer);
   uint8_t send[1012];
         std::fill(send, send+1012, 0x00);
  //  uint64_t numwant;
  int event;
  std::string temp;

  switch(action) {
    case(0):
      std::copy(buffer,buffer+size,send);
      //do things based on a connect
      send[0] = 0; //first bit signifies connect, in real torrent other information also added
      break;
    case(1):
      //do things based on an announce
      event = buffer[83]; //might not need
      // std::fill(buffer,buffer+buffer.size(), 0x00);

      send[0] = 1; //announce
      if (event==3) {
	temp = std::string((char*) buffer+84);
	RemoveTorrent(from,temp);
        return;
      } else if (event==2) {
      //TODO - change the stuff with the buffer add
        temp = std::string((char*) buffer+11);
        temp = AddTorrentReturnPeers(from, temp);
	std::copy(temp.c_str(), temp.c_str()+temp.size(), send+1);
      } else {
        NS_LOG_INFO("Doing nothing");
        return;
      }      
      break;
    case(2):
      //do things based on a scrape might not do this?
      NS_LOG_INFO("Scrape request in a normal torrent. We do nothing.");
      break;
   }
  //  m_socket->Connect (from);
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  Ptr<Packet> p = CreateReplyPacket(send, 1012);
  p->AddHeader(seqTs);
  m_sent++;
  if (m_socket->SendTo (p,0,from))
    {
      NS_LOG_INFO ("TraceDelay TX " << 1012 << " bytes to "
                                    << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " Port: "
                   << InetSocketAddress::ConvertFrom (from).GetPort () << " Uid: "
                                    << p->GetUid () << " Time: "
                                    << (Simulator::Now ()).GetSeconds ());

    }
  else
    {
      NS_LOG_INFO ("Error while sending " << 125 << " bytes to "
		   << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " " << InetSocketAddress::IsMatchingType(from)<<" "
                     << InetSocketAddress::ConvertFrom (from).GetPort());
    }

  //  m_socket->SendTo(p,0,from);
}
  
void
P2PServer::StartApplication (void)
{
 Ptr<Ipv4> ipv4 = this->m_node->GetObject<Ipv4>();
    Ipv4Address  m_localIpv4  =ipv4->GetAddress(1,0).GetLocal();
  //TODO server needs to be added to list...means we need to keep track of it somehow?
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO( "Starting Server" << (Simulator::Now ()).GetSeconds () << m_localIpv4);
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (),
                                                   m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&P2PServer::HandleRead, this));
  m_socket->SetAllowBroadcast(true);
  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (),
                                                   m_port);
      if (m_socket6->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }

  m_socket6->SetRecvCallback (MakeCallback (&P2PServer::HandleRead, this));

}

void
P2PServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void P2PServer::HandleRead (Ptr<Socket> socket)
{
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
                std::fill(buffer, buffer+size, 0x00);
          packet->CopyData(buffer, size);
          NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                          " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                       " Port: " << InetSocketAddress::ConvertFrom (from).GetPort () <<
                           " Sequence Number: " << currentSequenceNumber <<
                           " Uid: " << packet->GetUid () <<
                           " TXtime: " << seqTs.GetTs () <<
                           " RXtime: " << Simulator::Now () <<
                           " Delay: " << Simulator::Now () - seqTs.GetTs ());
          m_lossCounter.NotifyReceived (currentSequenceNumber);
          m_received++;
      	  Reply(from,packet);
        } 
    }
}

}
