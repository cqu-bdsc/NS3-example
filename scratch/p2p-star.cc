/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"

// Network topology (default)
//
//        n2 n3 n4              .
//         \ | /                .
//          \|/                 .
//     n1--- n0---n5            .
//          /|\                 .
//         / | \                .
//        n8 n7 n6              .
//


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Star");

uint32_t packet_count = 0;
void
TwoAddressTrace (std::string context, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr)
{
  packet_count++;
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds ();
  if (InetSocketAddress::IsMatchingType (destAddr))
      {
        InetSocketAddress addr = InetSocketAddress::ConvertFrom (destAddr);
        oss << " " << addr.GetIpv4 ();
      }

  if (InetSocketAddress::IsMatchingType (srcAddr))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (srcAddr);
      oss << " received one packet from " << addr.GetIpv4 ();
      uint8_t buffer[packet->GetSize() + 1];
      packet->CopyData(buffer, packet->GetSize());
      buffer[packet->GetSize()] = '\0';
      oss << ". data: " << buffer;
    }
  else
    {
      oss << " received one packet!";
    }
  NS_LOG_UNCOND(oss.str ());
}

int 
main (int argc, char *argv[])
{

  const std::string PROTOCOL = "ns3::UdpSocketFactory";

  //
  // Set up some default values for the simulation.
  //
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (137));

  // ??? try and stick 15kb/s into the data rate
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("14kb/s"));

  //
  // Default number of nodes in the star.  Overridable by command line argument.
  //
  uint32_t nSpokes = 8;

  CommandLine cmd;
  cmd.AddValue ("nSpokes", "Number of nodes to place in the star", nSpokes);
  cmd.Parse (argc, argv);

  NS_LOG_INFO ("Build star topology.");
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2000Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  PointToPointStarHelper star (nSpokes, pointToPoint);

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  star.InstallStack (internet);

  NS_LOG_INFO ("Assign IP Addresses.");
  star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));

  NS_LOG_INFO ("Create applications.");
  //
  // Create a packet sink on the star "hub" to receive packets.
  // 
  uint16_t port = 50000;
  Address hubLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper (PROTOCOL, hubLocalAddress);
  ApplicationContainer hubApp = packetSinkHelper.Install (star.GetHub ());
  hubApp.Start (Seconds (1.0));
//  hubApp.Stop (Seconds (10.0));

  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < star.SpokeCount (); ++u)
    {
      PacketSinkHelper obuPacketSinkHelper (PROTOCOL, InetSocketAddress (Ipv4Address::GetAny(), 50000));
      serverApps.Add (obuPacketSinkHelper.Install (star.GetSpokeNode(u)));
    }
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/RxWithAddresses", MakeCallback (&TwoAddressTrace));
  serverApps.Start (Seconds (0.01));

  //
  // Create OnOff applications to send TCP to the hub, one on each spoke node.
  //
  OnOffHelper onOffHelper (PROTOCOL, Address ());
  onOffHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onOffHelper.SetAttribute ("DataRate", DataRateValue (DataRate ("200Mb/s")));
  onOffHelper.SetAttribute ("PacketSize", UintegerValue (1024));
  onOffHelper.SetAttribute ("MaxBytes", UintegerValue (1024 * 10));

  ApplicationContainer spokeApps;

//  for (uint32_t i = 0; i < star.SpokeCount (); ++i)
//    {
//      AddressValue remoteAddress (InetSocketAddress (star.GetHubIpv4Address (i), port));
//      onOffHelper.SetAttribute ("Remote", remoteAddress);
//      spokeApps.Add (onOffHelper.Install (star.GetSpokeNode (i)));
//    }
  AddressValue remoteAddress (InetSocketAddress (star.GetSpokeIpv4Address(1), port));
  onOffHelper.SetAttribute ("Remote", remoteAddress);
  spokeApps.Add (onOffHelper.Install (star.GetSpokeNode (0)));

  spokeApps.Start (Seconds (1.0));
//  spokeApps.Stop (Seconds (10.0));

  NS_LOG_INFO ("Enable static global routing.");
  //
  // Turn on global static routing so we can actually be routed across the star.
  //
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(10));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  std::cout << "packet count: " << packet_count << std::endl;

  return 0;
}
