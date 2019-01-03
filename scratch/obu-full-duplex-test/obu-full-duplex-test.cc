
#include <iostream>
#include <iomanip>

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/stats-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/wave-helper.h"
#include "my-onoff-application.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleOcb");

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address srcAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " node " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (srcAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (srcAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
      uint8_t buffer[packet->GetSize()];
      packet->CopyData(buffer, packet->GetSize());
      oss << ". data: " << buffer;
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

uint32_t packet_count = 0;
void
ReceivePacket (Ptr<Socket> socket)
{
  packet_count++;
  Ptr<Packet> packet;
  Address srcAddress;
  while ((packet = socket->RecvFrom (srcAddress)))
    {
//      uint32_t RxRoutingBytes = packet->GetSize ();

      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, srcAddress));
    }
}

uint32_t phy_tx_begin = 0;
uint32_t phy_rx_begin = 0;
uint32_t phy_tx_end = 0;
uint32_t phy_rx_end = 0;
uint32_t phy_tx_drop = 0;
uint32_t phy_rx_drop = 0;
uint32_t mac_tx = 0;
uint32_t mac_rx = 0;
uint32_t mac_tx_drop = 0;
uint32_t mac_rx_drop = 0;
void
PacketTrace (std::string context, Ptr< const Packet > packet)
{
  std::string traceName = context.substr(context.find_last_of ("/") + 1, context.size());
  if (traceName.compare("PhyTxBegin") == 0)
    {
      phy_tx_begin++;
    }
  else if (traceName.compare("PhyRxBegin") == 0)
    {
      phy_rx_begin++;
    }
  else if (traceName.compare("PhyTxEnd") == 0)
    {
      phy_tx_end++;
    }
  else if (traceName.compare("PhyRxEnd") == 0)
    {
      phy_rx_end++;
    }
  else if (traceName.compare("PhyTxDrop") == 0)
    {
      phy_tx_drop++;
    }
  else if (traceName.compare("PhyRxDrop") == 0)
    {
      phy_rx_drop++;
    }
  else if (traceName.compare("MacTx") == 0)
    {
      mac_tx++;
    }
  else if (traceName.compare("MacRx") == 0)
    {
      mac_rx++;
    }
  else if (traceName.compare("MacTxDrop") == 0)
    {
      mac_tx_drop++;
    }
  else if (traceName.compare("MacRxDrop") == 0)
    {
      mac_rx_drop++;
    }
  NS_LOG_UNCOND (traceName);
}

uint32_t phy_rx_drop_pkt_count = 0;
void
PhyRxDropPacket (std::string context, Ptr< const Packet > packet)
{
  phy_rx_drop_pkt_count++;
  NS_LOG_UNCOND ("PHY Rx context:" << context);
  NS_LOG_UNCOND ("PHY Rx drop");
}

uint32_t mac_tx_drop_pkt_count = 0;
void
MacTxDropPacket (std::string context, Ptr< const Packet > packet)
{
  mac_tx_drop_pkt_count++;
  NS_LOG_UNCOND ("MAC Tx drop");
}

uint32_t mac_rx_drop_pkt_count = 0;
void
MacRxDropPacket (std::string context, Ptr< const Packet > packet)
{
  mac_rx_drop_pkt_count++;
  NS_LOG_UNCOND ("MAC Rx drop");
}

void
TwoAddressTrace (std::string context, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr)
{
  packet_count++;
  Ptr<Packet> pktCopy = packet->Copy();
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds ();
  if (InetSocketAddress::IsMatchingType (srcAddr))
      {
        InetSocketAddress addr = InetSocketAddress::ConvertFrom (srcAddr);
        oss << ", src: " << addr.GetIpv4 ();
      }

  if (InetSocketAddress::IsMatchingType (destAddr))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (destAddr);
      oss << ", dest: " << addr.GetIpv4 ();

      FlowIdTag flowIdTag;
      pktCopy->RemovePacketTag(flowIdTag);
      oss << ", flowId: " << flowIdTag.GetFlowId();

      PacketSeqTag packetSeqTag;
      pktCopy->RemovePacketTag(packetSeqTag);
      oss << ", packeSeq: " << packetSeqTag.GetPacketSeq();

      uint8_t buffer[packet->GetSize() + 1];
      packet->CopyData(buffer, packet->GetSize());
      buffer[packet->GetSize()] = '\0';
      oss << ", data: " << buffer;
    }
  else
    {
      oss << " received one packet!";
    }
  NS_LOG_UNCOND(oss.str ());
}

static void
GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize, pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

uint32_t m_dataSize = 0; //!< packet payload size (must be equal to m_size)
uint8_t *m_data = 0; //!< packet payload data
void
SetFill (std::string fill)
{
  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);
}

void
SendData (Ptr<Socket> socket, std::string data)
{
  SetFill(data);
  Ptr<Packet> packet = Create<Packet> (m_data, m_dataSize);
  socket->Send (packet);
}

void
SendZeroFilled (Ptr<Socket> socket, uint32_t size)
{
  Ptr<Packet> packet = Create<Packet> (size);
  socket->Send (packet);
}

int main (int argc, char *argv[])
{
  std::string phyMode ("OfdmRate3MbpsBW10MHz");
  uint32_t nNode = 2; // bytes
  std::string rate = "27";
  std::string sendRate = "6";
  uint32_t packetSize = 972; // udp: 972, tcp: 1448
  uint32_t nPackets = 20;
  uint32_t v2vPort = 3000;
  double simTime = 100.0; // seconds
  double interval = 1.0; // seconds
  bool verbose = false;
  double txp = 29.0;
  double distance = 200;
  double intervalTime = 0.1;
//  double scheduleTime = 0.01;

  std::map<std::uint32_t, std::vector<std::uint8_t *>> cache;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("nNode", "number of nodes", nNode);
  cmd.AddValue ("rate", "Rate", rate);
  cmd.AddValue ("sendRate", "Rate", sendRate);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("nPackets", "number of packets generated", nPackets);
  cmd.AddValue ("simTime", "simulation time", simTime);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("txp", "txp", txp);
  cmd.AddValue ("distance", "distance", distance);
  cmd.AddValue ("intervalTime", "broadcast interval time", intervalTime);
  cmd.Parse (argc, argv);

//  LogComponentEnable ("UdpSocketImpl", LOG_LEVEL_INFO);
//  LogComponentEnable ("MyOnOffApplication", (enum LogLevel)(LOG_LEVEL_INFO|LOG_PREFIX_FUNC|LOG_PREFIX_NODE));
//  LogComponentEnable ("UdpSocketImpl", (enum LogLevel)(LOG_LEVEL_LOGIC|LOG_PREFIX_FUNC|LOG_PREFIX_NODE));
//  LogComponentEnable ("WifiPhy", (enum LogLevel)(LOG_LEVEL_DEBUG|LOG_PREFIX_FUNC|LOG_PREFIX_NODE));
//  LogComponentEnable ("NistErrorRateModel", (enum LogLevel)(LOG_LEVEL_LOGIC|LOG_PREFIX_FUNC|LOG_PREFIX_NODE));

  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  phyMode = "OfdmRate" + rate + "MbpsBW10MHz";
  std::cout << "phyMode:" << phyMode << std::endl;

  //Set Non-unicastMode rate to unicast mode
//  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer nodes;
  nodes.Create (nNode);

  //--------------------------------------------------------------------------------------------------------------------------

#define LossModel 1
#if LossModel == 1
  Ptr<RangePropagationLossModel> loss = CreateObject<RangePropagationLossModel> ();
  loss->SetAttribute("MaxRange", DoubleValue (450));
#elif LossModel == 2
  Ptr<FriisPropagationLossModel> loss = CreateObject<FriisPropagationLossModel> ();
  loss->SetAttribute("Frequency", DoubleValue (5.9e9));
#elif LossModel == 3
  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
#endif
  Ptr<YansWifiChannel> wifiChannel = CreateObject<YansWifiChannel> ();
  wifiChannel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());
  wifiChannel->SetPropagationLossModel (loss);

//  YansWifiChannelHelper wifiChannelHelper = YansWifiChannelHelper::Default ();
//  wifiChannelHelper.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
//  wifiChannelHelper.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (450));
//  Ptr<YansWifiChannel> wifiChannel = wifiChannelHelper.Create ();

#define WAVE 0
#if !WAVE
  YansWifiPhyHelper phy =  YansWifiPhyHelper::Default ();
  phy.SetChannel (wifiChannel);
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  NqosWaveMacHelper mac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
  if (verbose)
    {
      wifi80211p.EnableLogComponents ();      // Turn on all Wifi 802.11p logging
    }
  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue (phyMode),
                                      "ControlMode",StringValue (phyMode));
#else
  YansWavePhyHelper phy =  YansWavePhyHelper::Default ();
  phy.SetChannel (wifiChannel);
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  QosWaveMacHelper mac = QosWaveMacHelper::Default ();
  WaveHelper waveHelper = WaveHelper::Default ();
  waveHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue (phyMode),
                                      "ControlMode",StringValue (phyMode),
				      "NonUnicastMode", StringValue (phyMode));
#endif

  // Set Tx Power
  phy.Set ("TxPowerStart",DoubleValue (29));
  phy.Set ("TxPowerEnd", DoubleValue (29));

#if !WAVE
//  NetDeviceContainer devices_80211p_TX = wifi80211p.Install (phy, mac, nodes);
  NetDeviceContainer devices_80211p_RX = wifi80211p.Install (phy, mac, nodes);
#else
//  NetDeviceContainer devices_80211p_TX = waveHelper.Install (phy, mac, nodes);
  NetDeviceContainer devices_80211p_RX = waveHelper.Install (phy, mac, nodes);
#endif

  // Adjust Tx Power 1
//  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice>(devices_80211p.Get(2));
//  device->GetPhy()->SetTxPowerStart(txp);
//  device->GetPhy()->SetTxPowerEnd(txp);

//  // Adjust Tx Power 2
//  Config::Set("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Phy/TxPowerStart", DoubleValue (txp));
//  Config::Set("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Phy/TxPowerEnd", DoubleValue (txp));

  MobilityHelper mobility;
  {
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
//    positionAlloc->Add (Vector (0.0, 0.0, 0.0));
//    positionAlloc->Add (Vector (distance, 0.0, 0.0));
//  //  positionAlloc->Add (Vector (0.0, distance, 0.0));
//    positionAlloc->Add (Vector (distance * 2, 0.0, 0.0));

    double delta = 10;
    for (uint32_t i =0; i < nodes.GetN(); i++)
      {
        positionAlloc->Add (Vector (i * delta, 0.0, 0.0));
      }

    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (nodes);
  }

  InternetStackHelper internet;
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
//  Ipv4InterfaceContainer interface_80211p_TX = ipv4.Assign (devices_80211p_TX);
  Ipv4InterfaceContainer interface_80211p_RX = ipv4.Assign (devices_80211p_RX);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
//  for (uint32_t i =0; i < nodes.GetN(); i++)
//    {
//      InetSocketAddress local = InetSocketAddress ("10.1.1.0", port);
//      Ptr<Socket> recvSink = Socket::CreateSocket (nodes.Get (i), tid);
//      recvSink->Bind (local);
//      recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
//    }

  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < nodes.GetN (); ++u)
    {
//      PacketSinkHelper obuPacketSinkHelper_Tx ("ns3::UdpSocketFactory", InetSocketAddress (interface_80211p_TX.GetAddress(u), v2vPort));
      PacketSinkHelper obuPacketSinkHelper_Rx ("ns3::UdpSocketFactory", InetSocketAddress (interface_80211p_RX.GetAddress(u), v2vPort));
//      PacketSinkHelper obuPacketSinkHelper_Rx ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(), v2vPort));
//      PacketSinkHelper obuPacketSinkHelper_Rx ("ns3::UdpSocketFactory", InetSocketAddress ("10.1.1.0", v2vPort));
//      serverApps.Add (obuPacketSinkHelper_Tx.Install (nodes.Get(u)));
      serverApps.Add (obuPacketSinkHelper_Rx.Install (nodes.Get(u)));
    }

#define Enable_Lte 0
#if Enable_Lte
  uint32_t ltePort = 4000;

  NodeContainer remoteNodes;
  remoteNodes.Create (1);

  {
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (distance / 2, distance / 2, 0.0));

    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (remoteNodes);
  }

  NetDeviceContainer devices_Obu_Lte = wifi80211p.Install (phy, mac, nodes);
  NetDeviceContainer devices_Remote_Lte = wifi80211p.Install (phy, mac, remoteNodes);

  internet.Install (remoteNodes);
  ipv4.SetBase ("10.1.10.0", "255.255.255.0");
  Ipv4InterfaceContainer interface_Remote_Lte = ipv4.Assign (devices_Remote_Lte);
  Ipv4InterfaceContainer interface_Obu_Lte = ipv4.Assign (devices_Obu_Lte);
  for (uint32_t u = 0; u < nodes.GetN (); ++u)
    {
      PacketSinkHelper ltePacketSinkHelper_Rx ("ns3::UdpSocketFactory", InetSocketAddress ("10.1.10.0", ltePort));
      serverApps.Add (ltePacketSinkHelper_Rx.Install (nodes.Get(u)));
    }
#endif

  serverApps.Start (Seconds (0.01));

#define PHY_MAC_TRACE 0
#if PHY_MAC_TRACE
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WaveNetDevice/Phy/PhyTxBegin", MakeCallback (&PacketTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WaveNetDevice/Phy/PhyRxBegin", MakeCallback (&PacketTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WaveNetDevice/Phy/PhyTxEnd", MakeCallback (&PacketTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WaveNetDevice/Phy/PhyRxEnd", MakeCallback (&PacketTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WaveNetDevice/Phy/PhyTxDrop", MakeCallback (&PacketTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WaveNetDevice/Phy/PhyRxDrop", MakeCallback (&PacketTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin", MakeCallback (&PacketTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxBegin", MakeCallback (&PacketTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxEnd", MakeCallback (&PacketTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd", MakeCallback (&PacketTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback (&PacketTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback (&PacketTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback (&PacketTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRx", MakeCallback (&PacketTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback (&PacketTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRxDrop", MakeCallback (&PacketTrace));
#endif

  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/RxWithAddresses", MakeCallback (&TwoAddressTrace));

#if 0
  {
//    Ptr<Socket> source = Socket::CreateSocket (nodes.Get (0), tid);
//    InetSocketAddress local = InetSocketAddress (interface_80211p_TX.GetAddress(0), port);
//    if (source->Bind (local) == -1)
//      {
//        NS_FATAL_ERROR ("Failed to bind socket");
//      }
//    InetSocketAddress remote = InetSocketAddress (interface_80211p_TX.GetAddress(1), port);
//    source->Connect (remote);
//    source->SetAllowBroadcast (true);
//
//    Simulator::Schedule(Seconds(1.0), &SendZeroFilled, source, packetSize);

//    Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
//				    Seconds (1.0), &GenerateTraffic,
//				    source, packetSize, numPackets, interPacketInterval);


    Ptr<MyOnOffApplication> onOffApp = CreateObject<MyOnOffApplication>();
    onOffApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
    onOffApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//    InetSocketAddress local = InetSocketAddress (interface_80211p_TX.GetAddress(0), 5000);
//    onOffApp->SetAttribute ("Local", AddressValue (local));
    InetSocketAddress remote = InetSocketAddress (interface_80211p_RX.GetAddress(1), v2vPort);
//    InetSocketAddress remote = InetSocketAddress ("10.1.1.255", v2vPort);
    onOffApp->SetAttribute ("Remote", AddressValue (remote));
    onOffApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName ("ns3::UdpSocketFactory")));
    std::ostringstream oss;
    oss << sendRate << "Mb/s";
    onOffApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
    onOffApp->SetAttribute ("PacketSize", UintegerValue (packetSize));
    onOffApp->SetAttribute ("MaxBytes", UintegerValue (packetSize * nPackets));
    onOffApp->SetStartTime (Seconds(1.0));
    nodes.Get(0)->AddApplication(onOffApp);
  }
#endif

#if 0
  {
    Ptr<MyOnOffApplication> onOffApp = CreateObject<MyOnOffApplication>();
    onOffApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
    onOffApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//    InetSocketAddress local = InetSocketAddress (interface_80211p_TX.GetAddress(1), localPort);
//    onOffApp->SetAttribute ("Local", AddressValue (local));
    InetSocketAddress remote = InetSocketAddress (interface_80211p_RX.GetAddress(2), v2vPort);
//    InetSocketAddress remote = InetSocketAddress ("10.1.1.255", remotePort + 1);
    onOffApp->SetAttribute ("Remote", AddressValue (remote));
    onOffApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName ("ns3::UdpSocketFactory")));
    std::ostringstream oss;
    oss << sendRate << "Mb/s";
    onOffApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
    onOffApp->SetAttribute ("PacketSize", UintegerValue (packetSize));
    onOffApp->SetAttribute ("MaxBytes", UintegerValue (packetSize * nPackets));
    onOffApp->SetStartTime (Seconds(1.0));
    nodes.Get(0)->AddApplication(onOffApp);
  }
#endif

#if 1
  for (uint32_t i = 1; i < nNode; ++i)
    {
      Ptr<MyOnOffApplication> onOffApp = CreateObject<MyOnOffApplication>();
      onOffApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
      onOffApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  //    InetSocketAddress local = InetSocketAddress (interface_80211p_TX.GetAddress(1), localPort);
  //    onOffApp->SetAttribute ("Local", AddressValue (local));
      InetSocketAddress remote = InetSocketAddress (interface_80211p_RX.GetAddress(i), v2vPort);
  //    InetSocketAddress remote = InetSocketAddress ("10.1.1.255", remotePort + 1);
      onOffApp->SetAttribute ("Remote", AddressValue (remote));
      onOffApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName ("ns3::UdpSocketFactory")));
      std::ostringstream oss;
      oss << sendRate << "Mb/s";
      onOffApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
      onOffApp->SetAttribute ("PacketSize", UintegerValue (packetSize));
      onOffApp->SetAttribute ("MaxBytes", UintegerValue (packetSize * nPackets));
      onOffApp->SetStartTime (Seconds(1.0));
      nodes.Get(0)->AddApplication(onOffApp);
    }
#endif

#if 0
  {
    Ptr<MyOnOffApplication> onOffApp = CreateObject<MyOnOffApplication>();
    onOffApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
    onOffApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//    InetSocketAddress local = InetSocketAddress (interface_80211p_TX.GetAddress(1), remotePort);
//    onOffApp->SetAttribute ("Local", AddressValue (local));
    InetSocketAddress remote = InetSocketAddress (interface_80211p_RX.GetAddress(1), v2vPort);
    onOffApp->SetAttribute ("Remote", AddressValue (remote));
    onOffApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName ("ns3::UdpSocketFactory")));
    std::ostringstream oss;
    oss << sendRate << "Mb/s";
    onOffApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
    onOffApp->SetAttribute ("PacketSize", UintegerValue (packetSize));
    onOffApp->SetAttribute ("MaxBytes", UintegerValue (packetSize * nPackets));
    onOffApp->SetStartTime(Seconds(1.0));
    nodes.Get(2)->AddApplication(onOffApp);
  }
#endif

#if 0
  {
    Ptr<MyOnOffApplication> onOffApp = CreateObject<MyOnOffApplication>();
    onOffApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
    onOffApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    InetSocketAddress remote = InetSocketAddress ("10.1.10.255", ltePort);
    onOffApp->SetAttribute ("Remote", AddressValue (remote));
    onOffApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName ("ns3::UdpSocketFactory")));
    std::ostringstream oss;
    oss << sendRate << "Mb/s";
    onOffApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
    onOffApp->SetAttribute ("PacketSize", UintegerValue (packetSize));
    onOffApp->SetAttribute ("MaxBytes", UintegerValue (packetSize * nPackets));
    onOffApp->SetStartTime(Seconds(1.0));
    remoteNodes.Get(0)->AddApplication(onOffApp);
  }
#endif

#if 0
  {
//    UdpClientHelper client (Ipv4Address("10.1.1.255"), v2vPort);
    UdpClientHelper client (interface_80211p_RX.GetAddress(1), v2vPort);
    client.SetAttribute ("MaxPackets", UintegerValue (nPackets));
    client.SetAttribute ("Interval", TimeValue (Time ("0.01"))); //packets/s
    client.SetAttribute ("PacketSize", UintegerValue (packetSize));
    ApplicationContainer clientApp = client.Install (nodes.Get(0));
    clientApp.Start (Seconds (1.0));
  }
#endif

  Simulator::Stop(Seconds(simTime));
  Simulator::Run ();
  Simulator::Destroy ();

  std::cout.setf(std::ios::left);
  std::cout << std::setw(15) << "packet count: " << packet_count << std::endl;
#if PHY_MAC_TRACE
  std::cout << std::setw(15) << "PHY Tx begin: " << phy_tx_begin << std::endl;
  std::cout << std::setw(15) << "PHY Rx begin: " << phy_rx_begin << std::endl;
  std::cout << std::setw(15) << "PHY Tx end: " << phy_tx_end << std::endl;
  std::cout << std::setw(15) << "PHY Rx end: " << phy_rx_end << std::endl;
  std::cout << std::setw(15) << "PHY Tx drop: " << phy_tx_drop << std::endl;
  std::cout << std::setw(15) << "PHY Rx drop: " << phy_rx_drop << std::endl;
  std::cout << std::setw(15) << "MAC Tx: " << mac_tx << std::endl;
  std::cout << std::setw(15) << "MAC Rx: " << mac_rx << std::endl;
  std::cout << std::setw(15) << "MAC Tx drop: " << mac_tx_drop << std::endl;
  std::cout << std::setw(15) << "MAC Rx drop: " << mac_rx_drop << std::endl;
#endif

  return 0;
}
