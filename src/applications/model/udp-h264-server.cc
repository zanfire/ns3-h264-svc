/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */

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
#include "packet-loss-counter.h"

//#include "seq-ts-header.h"
#include "h264-trace-header.h"
//#include "udp-server.h"
#include "udp-h264-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpH264Server");
NS_OBJECT_ENSURE_REGISTERED (UdpH264Server);


TypeId
UdpH264Server::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpH264Server")
    .SetParent<Application> ()
    .AddConstructor<UdpH264Server> ()
    .AddAttribute ("Port",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&UdpH264Server::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketWindowSize",
                   "The size of the window used to compute the packet loss. This value should be a multiple of 8.",
                   UintegerValue (32),
                   MakeUintegerAccessor (&UdpH264Server::GetPacketWindowSize,
                                         &UdpH264Server::SetPacketWindowSize),
                   MakeUintegerChecker<uint16_t> (8,256))
  ;
  return tid;
}

UdpH264Server::UdpH264Server ()
  : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received=0;
}

UdpH264Server::~UdpH264Server ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
UdpH264Server::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
UdpH264Server::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
UdpH264Server::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetLost ();
}

uint32_t
UdpH264Server::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
}

void
UdpH264Server::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
UdpH264Server::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (),
                                                   m_port);
      m_socket->Bind (local);
    }

  m_socket->SetRecvCallback (MakeCallback (&UdpH264Server::HandleRead, this));

  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (),
                                                   m_port);
      m_socket6->Bind (local);
    }

  m_socket6->SetRecvCallback (MakeCallback (&UdpH264Server::HandleRead, this));

}

void
UdpH264Server::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
UdpH264Server::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () > 0)
        {
          //NS_LOG_INFO ("Got packet of size " <<  packet->GetSize () << " at " << Simulator::Now());
          H264TraceHeader h264header;
          packet->RemoveHeader (h264header);
          //uint32_t currentSequenceNumber = seqTs.GetSeq ();
          /*
          Input格式
             <Transmit Time>\t<Frame Size>\t<Lid>\t<Tid>\t<Qid>\t<Frame Number>
 
          Otput格式
             <Receive Time>\t<Frame Size>\t<Lid>\t<Tid>\t<Qid>\t<Frame Number>
          */
          if (InetSocketAddress::IsMatchingType (from))
            {
              /*
              NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                           " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                           " Sequence Number: " << currentSequenceNumber <<
                           " Uid: " << packet->GetUid () <<
                           " TXtime: " << seqTs.GetTs () <<
                           " RXtime: " << Simulator::Now () <<
                           " Delay: " << Simulator::Now () - seqTs.GetTs ());
              */
              NS_LOG_INFO ("[VIDEO] " << Simulator::Now () << "\t" 
                                      << h264header.GetSize() << "\t"
                                      << h264header.GetLid() << "\t"
                                      << h264header.GetTid() << "\t"
                                      << h264header.GetQid() << "\t"
                                      << h264header.GetFrameNo() << "\t"
                          );
            }
          else if (Inet6SocketAddress::IsMatchingType (from))
            {
              /*
              NS_LOG_INFO ("TraceDelay: RX " << packet->GetSize () <<
                           " bytes from "<< Inet6SocketAddress::ConvertFrom (from).GetIpv6 () <<
                           " Sequence Number: " << currentSequenceNumber <<
                           " Uid: " << packet->GetUid () <<
                           " TXtime: " << seqTs.GetTs () <<
                           " RXtime: " << Simulator::Now () <<
                           " Delay: " << Simulator::Now () - seqTs.GetTs ());
              */
              NS_LOG_INFO ("[VIDEO] " << Simulator::Now () << "\t" 
                                      << h264header.GetSize() << "\t"
                                      << h264header.GetLid() << "\t"
                                      << h264header.GetTid() << "\t"
                                      << h264header.GetQid() << "\t"
                                      << h264header.GetFrameNo() << "\t"
                          );
            }

          //m_lossCounter.NotifyReceived (currentSequenceNumber);
          m_received++;
        }
    }
}

} // Namespace ns3
