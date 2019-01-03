/*
 * vanet-facs.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: Yang yanning <yang.ksn@gmail.com>
 */

#include "vanet-facs.h"

using namespace ns3;

VanetCsVfcExperiment::VanetCsVfcExperiment ():
    m_protocol (0),
    m_dlPort (1000),
    m_ulPort (2000),
    m_v2IPort (3000),
    m_i2IPort (4000),
    m_i2VPort (5000),
    m_v2VPort (6000),
    m_phyMode ("OfdmRate6MbpsBW10MHz"),
    m_traceMobility (false),
    m_log (1),
    m_totalSimTime (0.0),
    m_verbose (0),
    m_routingTables (0),
    m_asciiTrace (0),
    m_pcap (0),
    m_timeSpent (0.0),
    m_currentBroadcastId (0),
    m_dataPktId (0),
    m_receiveCount (0)
{
}

VanetCsVfcExperiment::~VanetCsVfcExperiment ()
{
  libZipf_RequestTerminate();
  if (m_schemeName.compare(Scheme_1) == 0)
    {
      libFACSTerminate();
    }
  else if (m_schemeName.compare(Scheme_2) == 0)
    {
      libNCBTerminate();
    }
  else if (m_schemeName.compare(Scheme_3) == 0)
    {
      libISXDTerminate();
    }
  mclTerminateApplication();
}

void
VanetCsVfcExperiment::Simulate (int argc, char **argv)
{
  NS_LOG_UNCOND ("Start Simulate...");

  SetDefaultAttributeValues ();
  ParseCommandLineArguments (argc, argv);
  ConfigureNodes ();
//  ConfigureChannels ();
  ConfigureDevices ();
//  ConfigureInternet ();
//  ConfigureMobility ();
  ConfigureApplications ();
  ConfigureTracing ();
  RunSimulation ();
  ProcessOutputs ();
}

int
VanetCsVfcExperiment::MkPath(std::string s,mode_t mode=0755)
{
  size_t pre=0,pos;
  std::string dir;
  int mdret = 0;

  if(s[s.size()-1]!='/'){
      // force trailing / so we can handle everything in loop
      s+='/';
  }

  while((pos=s.find_first_of('/',pre))!=std::string::npos){
      dir=s.substr(0,pos++);
      pre=pos;
      if(dir.size()==0) continue; // if leading / first time is 0 length
      if((mdret=::mkdir(dir.c_str(),mode)) && errno!=EEXIST){
          return mdret;
      }
  }
  return mdret;
}

void
VanetCsVfcExperiment::SetDefaultAttributeValues ()
{
  // handled in constructor
}

void
VanetCsVfcExperiment::ParseCommandLineArguments (int argc, char **argv)
{
  CommandSetup (argc, argv);
  Initialization ();

  ConfigureDefaults ();
}

void
VanetCsVfcExperiment::ConfigureNodes ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_obuNodes.Create (m_nObuNodes);
  m_rsuNodes.Create (m_nRsuNodes);

  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      m_vehId2IndexMap.insert(make_pair(m_obuNodes.Get(i)->GetId(), i));
    }
  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      m_fogId2FogIdxMap.insert(make_pair(m_rsuNodes.Get(i)->GetId(), i));
    }
}

void
VanetCsVfcExperiment::ConfigureChannels ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
VanetCsVfcExperiment::ConfigureDevices ()
{
  NS_LOG_FUNCTION_NOARGS ();

  SetupDevices ();
  // devices are set up in SetupAdhocDevices(),
  // called by ConfigureChannels()

  // every device will have PHY callback for tracing
  // which is used to determine the total amount of
  // data transmitted, and then used to calculate
  // the MAC/PHY overhead beyond the app-data								// get the raw pointer
  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/Tx", MakeCallback (&WifiPhyStats::PhyTxTrace, m_wifiPhyStats)); // @suppress("Invalid arguments")
  // TxDrop, RxDrop not working yet.  Not sure what I'm doing wrong.
//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback (&WifiPhyStats::PhyTxDrop, m_wifiPhyStats));
//  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback (&WifiPhyStats::PhyRxDrop, m_wifiPhyStats));
}

void
VanetCsVfcExperiment::ConfigureInternet ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
VanetCsVfcExperiment::ConfigureMobility ()
{
  NS_LOG_FUNCTION_NOARGS ();

  SetupObuMobilityNodes ();
  SetupRsuMobilityNodes ();
}
void
VanetCsVfcExperiment::ConfigureApplications ()
{
  NS_LOG_FUNCTION_NOARGS ();

  // config trace to capture app-data (bytes) for
  // routing data, subtracted and used for
  // routing overhead
  std::ostringstream oss;
  oss.str ("");
  oss << "/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx";
  Config::Connect (oss.str (), MakeCallback (&VanetCsVfcExperiment::OnOffTrace, this));

  ApplicationContainer sinkApps;
  for (uint32_t i = 0; i < m_nObuNodes; ++i)
    {
#if Lte_Enable
      PacketSinkHelper obuLteDlSinkHelper (PROTOCOL_UDP, InetSocketAddress (m_ueInterface.GetAddress(i), m_dlPort));
      sinkApps.Add (obuLteDlSinkHelper.Install (m_ueNodes.Get(i)));
#else
      PacketSinkHelper obuC2VSinkHelper (PROTOCOL_UDP, InetSocketAddress ("10.2.0.0", m_dlPort));
      sinkApps.Add (obuC2VSinkHelper.Install (m_ueNodes.Get(i)));
#endif
//      PacketSinkHelper obuI2VSinkHelper (PROTOCOL_UDP, InetSocketAddress ("10.3.0.0", m_i2VPort));
      PacketSinkHelper obuI2VSinkHelper (PROTOCOL_UDP, InetSocketAddress (m_obuInterfacesRx.GetAddress(i), m_i2VPort));
      sinkApps.Add (obuI2VSinkHelper.Install (m_obuNodes.Get(i)));

//      PacketSinkHelper obuV2VSinkHelper (PROTOCOL_UDP, InetSocketAddress ("10.3.0.0", m_v2VPort));
      PacketSinkHelper obuV2VSinkHelper (PROTOCOL_UDP, InetSocketAddress (m_obuInterfacesRx.GetAddress(i), m_v2VPort));
      sinkApps.Add (obuV2VSinkHelper.Install (m_obuNodes.Get(i)));
    }
  for (uint32_t i = 0; i < m_nRsuNodes; ++i)
    {
      PacketSinkHelper rsuV2ISinkHelper (PROTOCOL_UDP, InetSocketAddress (m_rsu80211pInterfacesRx.GetAddress(i), m_v2IPort));
      sinkApps.Add (rsuV2ISinkHelper.Install (m_rsuNodes.Get(i)));

      PacketSinkHelper rsuI2ISinkHelper (PROTOCOL_UDP, InetSocketAddress (m_rsuStarHelper.GetSpokeIpv4Address(i), m_i2IPort));
//      PacketSinkHelper rsuI2ISinkHelper (PROTOCOL_UDP, InetSocketAddress ("192.168.0.0", m_i2IPort));
      sinkApps.Add (rsuI2ISinkHelper.Install (m_rsuNodes.Get(i)));
    }
#if Cloud_Enable
  PacketSinkHelper remoteHostSinkHelper (PROTOCOL_UDP, InetSocketAddress (m_remoteHostAddr, m_ulPort));
  sinkApps.Add (remoteHostSinkHelper.Install (m_remoteHost));
#endif
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/RxWithAddresses", MakeCallback (&VanetCsVfcExperiment::ReceivePacketWithAddr, this));
  sinkApps.Start (Seconds (0.01));
}

void
VanetCsVfcExperiment::ConfigureTracing ()
{
  NS_LOG_FUNCTION_NOARGS ();

//  WriteCsvHeader ();
  SetupTraceFile ();
  SetupLogging ();
}

void
VanetCsVfcExperiment::ConfigureAnim ()
{
  NS_LOG_FUNCTION_NOARGS ();

  AnimationInterface anim (m_animFile);
  uint32_t resId1 = anim.AddResource("/home/haha/Pictures/icon/car-003.png");
  uint32_t resId3 = anim.AddResource("/home/haha/Pictures/icon/rsu-001.png");
  uint32_t resId5 = anim.AddResource("/home/haha/Pictures/icon/cloud.png");

  for (uint32_t i = 0; i < m_nObuNodes; ++i)
    {
      Ptr<Node> obu = m_obuNodes.Get(i);
//      anim.UpdateNodeDescription (obu, "OBU");
      anim.UpdateNodeSize(obu->GetId(), 50, 50);
      anim.UpdateNodeImage(obu->GetId(), resId1);
    }
  for (uint32_t i = 0; i < m_nRsuNodes; ++i)
    {
      Ptr<Node> rsu = m_rsuNodes.Get(i);
//      anim.UpdateNodeDescription (rsu, "RSU");
      anim.UpdateNodeSize(rsu->GetId(), 50, 50);
      anim.UpdateNodeImage(rsu->GetId(), resId3);
    }
#if Lte_Enable
  uint32_t resId2 = anim.AddResource("/home/haha/Pictures/icon/cellular-001.png");
  uint32_t resId4 = anim.AddResource("/home/haha/Pictures/icon/pgw.png");
  for (uint32_t i = 0; i < m_enbNodes.GetN(); ++i)
    {
      Ptr<Node> enb = m_enbNodes.Get(i);
//      anim.UpdateNodeDescription (enb, "eNB");
      anim.UpdateNodeSize(enb->GetId(), 50, 50);
      anim.UpdateNodeImage(enb->GetId(), resId2);
    }

//  anim.UpdateNodeDescription (m_pgw, "MME/PGW");
  anim.UpdateNodeSize(m_pgw->GetId(), 50, 50);
  anim.UpdateNodeImage(m_pgw->GetId(), resId4);
#endif

//  anim.UpdateNodeDescription (m_remoteHost, "Remote Host");
  anim.UpdateNodeSize(m_remoteHost->GetId(), 50, 50);
  anim.UpdateNodeImage(m_remoteHost->GetId(), resId5);

  anim.SetStartTime (Seconds(1));
  anim.SetStopTime (Seconds(10));
  anim.EnablePacketMetadata ();
//  anim.EnableIpv4RouteTracking ("routingtable-wireless.xml", Seconds (0), Seconds (5), Seconds (0.25));
//  anim.EnableWifiMacCounters (Seconds (0), Seconds (10));
//  anim.EnableWifiPhyCounters (Seconds (0), Seconds (10));
}

void
VanetCsVfcExperiment::RunSimulation ()
{
  NS_LOG_FUNCTION_NOARGS ();

#if Total_Time_Spent_stas
  clock_t startTime, endTime;
  startTime = clock();
  std::cout << "simulation start time " << Now() << " -- " << (double)(startTime) / CLOCKS_PER_SEC << std::endl;
#endif

  Run ();

#if Total_Time_Spent_stas
  endTime = clock();
  m_timeSpent = (double)(endTime - startTime) / CLOCKS_PER_SEC;
  std::cout << "simulation end time " << Now() << " -- " << m_timeSpent << std::endl;
#endif
}

void
VanetCsVfcExperiment::Run ()
{
  NS_LOG_FUNCTION_NOARGS ();

#if Gen_Animation
  ConfigureAnim ();
#endif

//  Simulator::Schedule(Seconds(1.0), &VanetCsVfcExperiment::Loop, this);
  Loop();
  Simulator::Schedule(Seconds(m_schedulingPeriod), &VanetCsVfcExperiment::DoScheduling, this);

  // testing
#if 0 //pint to point
  {
#undef Print_Received_Log
#define Print_Received_Log true
    Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
    dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
    dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    InetSocketAddress remote = InetSocketAddress (m_rsuStarHelper.GetSpokeIpv4Address(3), m_i2IPort);
    dataTxApp->SetAttribute ("Remote", AddressValue (remote));
    std::ostringstream oss;
    oss << Rate_F2F << "Mb/s";
    dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
    dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
    dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
    dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

    using vanet::PacketHeader;
    PacketHeader header;
    header.SetType(PacketHeader::MessageType::NOT_SET);
    header.SetDataPktId(++m_dataPktId);
    header.SetDataSize(m_dataSize);
    header.SetBroadcastId(m_currentBroadcastId);
    dataTxApp->SetHeader(header);

    using vanet::PacketTagV2v;
    PacketTagV2v *pktTagV2v = new PacketTagV2v();
    pktTagV2v->SetSrcNodeId(m_remoteHost->GetId());
    std::vector<uint32_t> dataIdxs;
    dataIdxs.push_back(23);
    pktTagV2v->SetDataIdxs(dataIdxs);
    dataTxApp->SetPacketTag(pktTagV2v);

    m_rsuNodes.Get (1)->AddApplication(dataTxApp);
    dataTxApp->SetStartTime(Seconds(2));
  }
#endif

#if 0 // v2v
  {
#undef Print_Received_Log
#define Print_Received_Log true
    uint32_t coVehIdx = 1;
    uint32_t vehIdx = 47;
    Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
    dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
    dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    InetSocketAddress remote = InetSocketAddress (m_obuInterfacesTx.GetAddress(vehIdx), m_v2VPort);
    dataTxApp->SetAttribute ("Remote", AddressValue (remote));
    std::ostringstream oss;
    oss << Rate_V2V << "Mb/s";
    dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
    dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
    dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
    dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

    using vanet::PacketHeader;
    PacketHeader header;
    header.SetType(PacketHeader::MessageType::NOT_SET);
    header.SetDataPktId(++m_dataPktId);
    header.SetDataSize(m_dataSize);
    header.SetBroadcastId(m_currentBroadcastId);
    dataTxApp->SetHeader(header);

    using vanet::PacketTagV2v;
    PacketTagV2v *pktTagV2v = new PacketTagV2v();
    pktTagV2v->SetSrcNodeId(m_remoteHost->GetId());
    std::vector<uint32_t> dataIdxs;
    dataIdxs.push_back(23);
    pktTagV2v->SetDataIdxs(dataIdxs);
    dataTxApp->SetPacketTag(pktTagV2v);

    m_obuNodes.Get (coVehIdx)->AddApplication(dataTxApp);
    dataTxApp->SetStartTime(Seconds(1));
  }
#endif

  Simulator::Stop (Seconds (m_totalSimTime));
  Simulator::Run ();
  Simulator::Destroy ();
}

void
VanetCsVfcExperiment::ProcessOutputs ()
{
  NS_LOG_FUNCTION_NOARGS ();

#if Print_Vehicle_Final_Request
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      if (m_vehsReqs[i].size() == 0) continue;
      cout << "veh:" << i << ", final reqs:";
      for(uint32_t req : m_vehsReqs[i])
	{
	  cout << " " << req;
	}
      cout << endl;
    }
#endif

#if Print_Vehicle_Final_Cache
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      if (m_vehsCaches[i].size() == m_globalDbSize) continue;
      cout << "veh:" << i << ", final caches:";
      for(uint32_t cache : m_vehsCaches[i])
	{
	  cout << " " << cache;
	}
      cout << endl;
    }
#endif

  int outWidth = 20;
  std::cout.setf(std::ios::right);
  std::cout << std::setw(outWidth) << "globalDbSize: " << m_globalDbSize << std::endl;
  std::cout << std::setw(outWidth) << "TimeSpent: " << m_timeSpent << std::endl;
  std::cout << std::setw(outWidth) << "receive_count: " << m_receiveCount << std::endl;
  std::cout << std::setw(outWidth) << "SubmittedReqs: " << m_pktsStats.GetSubmittedReqs() << std::endl;
  std::cout << std::setw(outWidth) << "SatisfiedReqs: " << m_pktsStats.GetSatisfiedReqs() << std::endl;
  std::cout << std::setw(outWidth) << "V2V/V2F: " << (m_pktsStats.GetV2vRxPkts() + m_pktsStats.GetV2fRxPkts()) << std::endl;
  std::cout << std::setw(outWidth) << "BroadcastPkts: " << m_pktsStats.GetBroadcastPkts() << std::endl;
  std::cout << std::setw(outWidth) << "CumulativeDelay: " << m_pktsStats.GetCumulativeDelay() << std::endl;

  double ASD = 0;
  double SR = 0;
  double BE = 0;
  if(m_pktsStats.GetSatisfiedReqs() != 0)
    {
      ASD = (m_pktsStats.GetCumulativeDelay() / 1e+9) / m_pktsStats.GetSatisfiedReqs();
      std::cout << std::setw(outWidth) << "ASD: " << ASD << "s" << std::endl;
    }

  if(m_pktsStats.GetSubmittedReqs() != 0)
    {
      SR = (double)m_pktsStats.GetSatisfiedReqs() / m_pktsStats.GetSubmittedReqs();
      std::cout << std::setw(outWidth) << "SR: " << SR << std::endl;
    }

  if(m_pktsStats.GetBroadcastPkts() != 0)
    {
      BE = (double)m_pktsStats.GetSatisfiedReqs() / m_pktsStats.GetBroadcastPkts();
      std::cout << std::setw(outWidth) << "BE: " << BE << std::endl;
    }
  std::cout << std::endl;

  if (m_isFileOut == 1)
    {
      std::string outFilePrefix = m_outputDir + "/" + m_schemeName + "-" + m_space + "-" + m_time + "-" + m_mapSize;
      std::ofstream ofs; ///< output file stream

      ofs.open (outFilePrefix + "-total.dat", ios::app);
      ofs << m_nObuNodes << " " << m_globalDbSize << " " << ASD << " " << SR << " " << BE << " "
	  << m_pktsStats.GetV2vRxPkts() << " " << m_pktsStats.GetV2fRxPkts() << std::endl;

      ofs.close ();
      ofs.open (outFilePrefix + "-eachVeh.dat", ios::app);

      double ASD1 = 0.0;
      for (uint32_t i = 0; i < m_nObuNodes; i++)
	{
	  ASD1 = (m_requestStatsEachVeh[i].GetCumulativeDelay() / 1e+9) / m_requestStatsEachVeh[i].GetSatisfiedReqs();
	  ofs << i << " " << m_requestStatsEachVeh[i].GetSubmittedReqs()
	      << " " << m_requestStatsEachVeh[i].GetSatisfiedReqs()
	      << " " << ASD1 << std::endl;
	}

      ofs.close ();
      ofs.open (outFilePrefix + "-everyPeriod.dat", ios::app);

      for (uint32_t i = 1; i <= m_currentBroadcastId; i++)
	{
	  ofs << i << " " << m_statsEveryPeriod[i][0]
	      << " " << m_statsEveryPeriod[i][1]
	      << " " << m_statsEveryPeriod[i][2] << std::endl;
	}

      ofs.close ();
    }

  m_ofs.close ();

#if Gen_Gnuplot_File
  Gnuplot plot = Gnuplot (m_plotFileName + ".png");
  plot.SetTerminal ("png");
  plot.SetLegend ("X Values", "Y Values");
  plot.AppendExtra("plot \"result1.dat\" using 1:2 w lp pt 5 title \"111\", \"result2.dat\" using 1:3 w lp pt 7 title \"222\"");

  // Open the plot file.
  std::ostringstream ossPltFile;
  ossPltFile << m_plotFileName << ".plt";
  std::ofstream plotFile (ossPltFile.str().c_str());
  plot.GenerateOutput (plotFile);
  // Close the plot file.
  plotFile.close ();
#endif
}

// Prints actual position and velocity when a course change event occurs
void
VanetCsVfcExperiment::CourseChange (std::string context, Ptr<const MobilityModel> mobility)
{
  Ptr<Node> obu = mobility->GetObject<Node> ();
  uint32_t obuId = obu->GetId();
  uint32_t obuIdx = m_vehId2IndexMap.at(obuId);

  m_nodesMoving[obuIdx] = true;

  // if the vehicle enters the observed area for the first time, it will initiate a request to the BS
//  vehsEnter[obuIdx] = true;
//  if (vehsEnter[obuIdx] == false)
//    {
//      vehsEnter[obuIdx] = true;
//    }

//  // Prints position and velocities
//  Vector pos_obu = mobility->GetPosition (); // Get position
//
//  std::cout << Simulator::Now () << "obuId:" << obuId << ", POS: x=" << pos_obu.x << ", y=" << pos_obu.y << std::endl;
}

void
VanetCsVfcExperiment::CourseChange (std::ostream *os, std::string context, Ptr<const MobilityModel> mobility)
{
  Vector pos_obu = mobility->GetPosition (); // Get position
  Vector vel_obu = mobility->GetVelocity (); // Get velocity

  pos_obu.z = 1.5;

  // Prints position and velocities
  *os << Simulator::Now () << " POS: x=" << pos_obu.x << ", y=" << pos_obu.y
      << ", z=" << pos_obu.z << "; VEL:" << vel_obu.x << ", y=" << vel_obu.y
      << ", z=" << vel_obu.z << std::endl;
}

void
VanetCsVfcExperiment::Loop()
{
  NS_LOG_FUNCTION_NOARGS ();

#if Upload_Enable
  UploadAllVehiclesInfo ();
#endif

  UpdateAllVehsStatus ();
  SubmitAllVehsRequests ();
  UpdateAllVehNeighbor ();
  UpdateAllFogCluster ();

#if Loop_Scheduling
  Simulator::Schedule(Seconds(m_schedulingPeriod), &VanetCsVfcExperiment::Loop, this);
#endif
}

void
VanetCsVfcExperiment::SubmitVehRequests(Ptr<Node> obu)
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t obuId = obu->GetId();
  uint32_t obuIdx = m_vehId2IndexMap.at(obuId);
  if (m_isFirstSubmit[obuIdx])
    {
      m_vehsReqs[obuIdx].insert(m_vehsInitialReqs[obuIdx].begin(), m_vehsInitialReqs[obuIdx].end());
      m_vehsCaches[obuIdx].insert(m_vehsInitialCaches[obuIdx].begin(), m_vehsInitialCaches[obuIdx].end());

      RequestStatus reqStatus;
      for (uint32_t reqIdx : m_vehsReqs[obuIdx])
	{
	  reqStatus.completed = false;
	  reqStatus.submitTime = Now().GetDouble();
	  m_vehsReqsStatus[obuIdx].insert(make_pair(reqIdx, reqStatus));

	  // NCB scheme
	  if (m_schemeName.compare(Scheme_2) == 0)
	    {
	      ReqQueueItem item(obuIdx, reqIdx);
	      m_requestQueue.push_back(item);
	    }
	}

      m_requestStatsEachVeh[obuIdx].IncSubmittedReqs(m_vehsReqs[obuIdx].size());
      m_pktsStats.IncSubmittedReqs(m_vehsReqs[obuIdx].size());

      m_isFirstSubmit[obuIdx] = false;
    }
}

void
VanetCsVfcExperiment::SubmitAllVehsRequests()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator i = m_obuNodes.Begin (); i != m_obuNodes.End (); ++i)
    {
      Ptr<Node> obu = (*i);
      uint32_t obuId = obu->GetId();
      uint32_t obuIdx = m_vehId2IndexMap.at(obuId);

      if (m_vehsEnter[obuIdx] == true)
        {
	  SubmitVehRequests(obu);
        }
    }
}

void
VanetCsVfcExperiment::UploadVehicleInfo(Ptr<Node> obu)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<MobilityModel> model = obu->GetObject<MobilityModel> ();
  Vector pos_obu = model->GetPosition ();

  uint32_t obuId = obu->GetId();
  uint32_t obuIdx = m_vehId2IndexMap.at(obuId);

  /**
   * vehicle id (uint32_t):		4 * byte
   * vehicle pos x (double):		8 * byte
   * vehicle pos y (double):		8 * byte
   * num of requsets (uint32_t):	4 * byte
   * vehicle request (uint32_t):	4 * byte * vehsReqs[obuIdx].size()
   * num of chaches (uint32_t):		4 * byte
   * vehicle cache (uint32_t):		4 * byte * vehsCaches[obuIdx].size()
   */
  uint32_t dataSize = 4 + 8 * 2 + 4 + 4 * m_vehsReqs[obuIdx].size() + 4 + 4 * m_vehsCaches[obuIdx].size();
  vanet::ByteBuffer bytes(dataSize);
  bytes.WriteU32(obuId);
  bytes.WriteDouble(pos_obu.x);
  bytes.WriteDouble(pos_obu.y);

  // the number of requests
  bytes.WriteU32(m_vehsReqs[obuIdx].size());
  for (uint32_t reqData : m_vehsReqs[obuIdx])
    {
      RequestStatus reqStats;
      reqStats.submitTime = Now().GetSeconds();
      m_vehsReqsStatus[obuIdx].insert(make_pair(reqData, reqStats));

      bytes.WriteU32(reqData);
    }

  // the number of cache
  bytes.WriteU32(m_vehsCaches[obuIdx].size());
  for (uint32_t cacheData : m_vehsCaches[obuIdx])
    {
      bytes.WriteU32(cacheData);
    }

  Ptr<UdpSender> sender = CreateObject<UdpSender>();
  sender->SetNode(obu);
  sender->SetRemote(m_remoteHostAddr, m_ulPort);
  sender->Start();
  using vanet::PacketHeader;
  PacketHeader header;
  header.SetType(PacketHeader::MessageType::REQUEST);
  sender->SetHeader(header);
  sender->SetFill(bytes.GetBufferData(), dataSize);

//  Simulator::ScheduleNow(&UdpSender::Send, sender);
  Simulator::Schedule(Seconds(0.02 + obuIdx*0.01), &UdpSender::Send, sender);
}

void
VanetCsVfcExperiment::UploadAllVehiclesInfo()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (NodeContainer::Iterator i = m_obuNodes.Begin (); i != m_obuNodes.End (); ++i)
    {
      Ptr<Node> obu = (*i);
      uint32_t obuId = obu->GetId();
      uint32_t obuIdx = m_vehId2IndexMap.at(obuId);

      if (m_vehsEnter[obuIdx] == true)
        {
	  UploadVehicleInfo(obu);
        }
      // for test
//      UploadVehicleInfo(obu);
    }
}

void
VanetCsVfcExperiment::UpdateAllVehsStatus()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      uint32_t obuIdx = i;
      // update the status of every vehicle
      if (m_nodesMoving[obuIdx])
	{
	  m_vehsEnter[obuIdx] = true;
	}
      else
	{
	  m_vehsEnter[obuIdx] = false;
	}
    }

  m_nodesMoving.clear();
  m_nodesMoving.resize(m_nObuNodes, false);

#if Print_Count_Of_Veh_In_Area
  uint32_t count = 0;
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      count += m_vehsEnter[i] ? 1 : 0;
    }
  std::cout << Simulator::Now() << " Number of vehicles in area: " << count << endl;
#endif
}

void
VanetCsVfcExperiment::UpdateAllVehNeighbor()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      uint32_t obuIdx1 = i;
      Ptr<Node> obu1 = m_obuNodes.Get(i);
      Ptr<MobilityModel> obuMobility1 = obu1->GetObject<MobilityModel> ();
      Vector pos_obu1 = obuMobility1->GetPosition ();

      for (uint32_t j = i + 1; j < m_nObuNodes; j++)
	{
	  uint32_t obuIdx2 = j;
	  Ptr<Node> obu2 = m_obuNodes.Get(obuIdx2);
	  Ptr<MobilityModel> obuMobility2 = obu2->GetObject<MobilityModel> ();
	  Vector pos_obu2 = obuMobility2->GetPosition ();
	  if (CalculateDistance (pos_obu1, pos_obu2) > m_obuTxRange || !m_vehsEnter[obuIdx1] || !m_vehsEnter[obuIdx2])
	    {
	      m_vehNeighbors[i][j] = 0;
	      m_vehNeighbors[j][i] = 0;
	    }
	  else
	    {
	      m_vehNeighbors[i][j] = 1;
	      m_vehNeighbors[j][i] = 1;
	    }
	}
    }
}

void
VanetCsVfcExperiment::UpdateAllFogCluster()
{
  NS_LOG_FUNCTION_NOARGS ();

//  Ptr<Node> bs = m_remoteHost;
//  Ptr<MobilityModel> bsMobility = bs->GetObject<MobilityModel> ();
//  Vector pos_bs = bsMobility->GetPosition ();

  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      uint32_t obuIdx = i;
#if Upload_Enable
      Vector pos_obu = m_vehsMobInfoInCloud[obuIdx];
#else
      Ptr<Node> obu = m_obuNodes.Get(i);
      Ptr<MobilityModel> obuMobility = obu->GetObject<MobilityModel> ();
      Vector pos_obu = obuMobility->GetPosition ();
#endif
      // update vehicle set for every fog
      for (uint32_t j = 0; j < m_nRsuNodes; j++)
	{
	  Ptr<Node> rsu = m_rsuNodes.Get(j);
	  Ptr<MobilityModel> rsuMobility = rsu->GetObject<MobilityModel> ();
	  Vector pos_rsu = rsuMobility->GetPosition ();
	  if (CalculateDistance (pos_obu, pos_rsu) > m_rsuTxRange || !m_vehsEnter[obuIdx])
	    {
	      m_fogCluster[j].erase(obuIdx);

	      std::map<uint32_t, uint32_t>::iterator iter = m_vehIdx2FogIdxMap.find(obuIdx);
	      if (iter != m_vehIdx2FogIdxMap.end() && iter->second == j)
		{
		  m_vehIdx2FogIdxMap.erase(obuIdx);
		}
	    }
	  else
	    {
	      m_fogCluster[j].insert(obuIdx);
	      m_vehIdx2FogIdxMap[obuIdx] = j;
	    }
	}
    }

  UpdateAllFogData();

#if Print_Total_Num_Veh_In_Fog
  uint32_t total = 0;
  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      total += m_fogCluster[i].size();
    }
  std::cout << Simulator::Now() << " Number of vehicles within the coverage of the fog node: " << total << endl;
#endif

#if Print_Fog_Cluster
  PrintFogCluster();
#endif //Print_Fog_Cluster
}

void
VanetCsVfcExperiment::UpdateRemainingCommuTime()
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t simTime = (uint32_t)m_totalSimTime;
  double nowF = Now().GetSeconds();
  uint32_t nowI = (uint32_t)ceil(nowF);

  // caculate remaining time for c2v
  for (uint32_t i = 0; i < m_nObuNodes; ++i)
    {
      if (m_vehsEnter[i])
	{
	  m_c2vRemainingTime[i] = m_vehsEnterExitTime[i][1] - nowF;
	}
    }

#if Print_Remaining_Time
  std::ofstream ofs;
  std::ostringstream oss;
  oss << m_cwd << "/v2c-remaining-time.txt";
  ofs.open(oss.str(), ios::out);

  for (uint32_t i = 0; i < m_nObuNodes; ++i)
    {
      ofs << " " << m_c2vRemainingTime[i];
    }
  ofs.close();

#endif //Print_Remaining_Time

  // caculate remaining time for v2v
  for (uint32_t i = 0; i < m_nObuNodes; ++i)
    {
      for (uint32_t j = i + 1; j < m_nObuNodes; ++j)
	{
	  if (m_vehNeighbors[i][j] == 1)
	    {
	      for (uint32_t t = nowI; t <= simTime; ++t)
		{
		  Vector pos_obu1 = m_nodeidxTimeLocation[i][t];
		  Vector pos_obu2 = m_nodeidxTimeLocation[j][t];
		  if (CalculateDistance (pos_obu1, pos_obu2) > m_obuTxRange
		      || t > m_vehsEnterExitTime[i][1]
		      || t > m_vehsEnterExitTime[j][1])
		    {
		      double remaining = (t - nowI) + ((double)nowI - nowF);
		      m_v2vRemainingTime[i][j] = remaining;
		      m_v2vRemainingTime[j][i] = remaining;
		      break;
		    }
		}
	    }
	}
    }

#if Print_Remaining_Time
  oss.str("");
  oss << m_cwd << "/v2v-remaining-time.txt";
  ofs.open(oss.str(), ios::out);
  ofs.setf(std::ios::left);
  for (uint32_t i = 0; i < m_nObuNodes; ++i)
    {
      for (uint32_t j = 0; j < m_nObuNodes; ++j)
	{
	  ofs << " " << std::setw(5) << m_v2vRemainingTime[i][j];
	}
      ofs << endl;
    }
  ofs.close();
#endif //Print_Remaining_Time

  // caculate remaining time for v2f
  for (uint32_t j = 0; j < m_nRsuNodes; ++j)
    {
      Ptr<Node> rsu = m_rsuNodes.Get(j);
      Ptr<MobilityModel> rsuMobility = rsu->GetObject<MobilityModel> ();
      Vector pos_rsu = rsuMobility->GetPosition ();
      for (uint32_t veh : m_fogCluster[j])
	{
	  for (uint32_t t = nowI; t <= simTime; ++t)
	    {
	      Vector pos_obu = m_nodeidxTimeLocation[veh][t];
	      if (CalculateDistance (pos_obu, pos_rsu) > m_rsuTxRange
		  || t > m_vehsEnterExitTime[veh][1])
		{
		  double remaining = (t - nowI) + ((double)nowI - nowF);
		  m_v2fRemainingTime[j][veh] = remaining;
		  break;
		}
	    }
	}
    }

#if Print_Remaining_Time
  oss.str("");
  oss << m_cwd << "/v2f-remaining-time.txt";
  ofs.open(oss.str(), ios::out);
  ofs.setf(std::ios::left);
  for (uint32_t j = 0; j < m_nRsuNodes; ++j)
    {
      for (uint32_t i = 0; i < m_nObuNodes; ++i)
	{
	  ofs << " " << std::setw(5) << m_v2fRemainingTime[j][i];
	}
      ofs << endl;
    }
  ofs.close();
#endif //Print_Remaining_Time
}

void
VanetCsVfcExperiment::DoScheduling()
{
  NS_LOG_FUNCTION_NOARGS ();

#if Scheduling
#if Print_Vehicle_Cache
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      if (m_vehsCaches[i].size() == 0) continue;
      cout << "veh:" << i;
      cout << ", caches:";
      for(uint32_t cache : m_vehsCaches[i])
	{
	  cout << " " << cache;
	}
      cout << endl;
    }
#endif // Print_Vehicle_Cache

  m_appContainer.Stop(Seconds(0.0));
  m_appContainer = ApplicationContainer();
  if (m_schemeName.compare(Scheme_1) == 0)
    {
      FacsFunc();
    }
  else if (m_schemeName.compare(Scheme_2) == 0)
    {
      NcbFunc();
    }
  else if (m_schemeName.compare(Scheme_3) == 0)
    {
      IsxdFunc();
    }
#endif // Scheduling
}

void
VanetCsVfcExperiment::UpdateAllFogData()
{
  NS_LOG_FUNCTION_NOARGS ();

#if Print_Vehicle_Request
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      if (m_vehsReqs[i].size() == 0) continue;
      cout << "veh:" << i;
      if (m_vehIdx2FogIdxMap.count(i))
	{
	  cout << ", fog:" << m_vehIdx2FogIdxMap.at(i);
	}
      else
	{
	  cout << ", fog:" << "none";
	}
      cout << ", reqs:";
      for(uint32_t req : m_vehsReqs[i])
	{
	  cout << " " << req;
	}
      cout << endl;
    }
#endif // Print_Vehicle_Request

  // update fog request set cache set
  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      m_fogsCaches[i].clear();
      m_fogsReqs[i].clear();
      std::set<uint32_t> vehSet = m_fogCluster[i];
      for (uint32_t veh : vehSet)
	{
#if Upload_Enable
	  m_fogsCaches[i].insert(m_vehsCachesInCloud[veh].begin(), m_vehsCachesInCloud[veh].end());
	  m_fogsReqs[i].insert(m_vehsReqsInCloud[veh].begin(), m_vehsReqsInCloud[veh].end());
#else
	  m_fogsCaches[i].insert(m_vehsCaches[veh].begin(), m_vehsCaches[veh].end());
	  m_fogsReqs[i].insert(m_vehsReqs[veh].begin(), m_vehsReqs[veh].end());
#endif // Upload_Enable
	}
    }
}

void
VanetCsVfcExperiment::PrintFogCluster()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_log != 0)
  {
    std::cout << Simulator::Now() << endl;
    for (uint32_t fogIdx = 0; fogIdx < m_nRsuNodes; fogIdx++)
      {
#if Print_Distance_OBU_RSU
	Ptr<Node> rsu = m_rsuNodes.Get(fogIdx);
	Ptr<MobilityModel> rsuMobility = rsu->GetObject<MobilityModel> ();
	Vector pos_rsu = rsuMobility->GetPosition ();
#endif // Print_Distance_OBU_RSU

	std::cout << "fogCluster[" << fogIdx << "]:";
#if 1
	for (uint32_t obuIdx : m_fogCluster[fogIdx])
	  {
	    std::cout << " " << obuIdx;
#if Print_Distance_OBU_RSU
	    Ptr<Node> obu = m_obuNodes.Get(obuIdx);
	    Ptr<MobilityModel> obuMobility = obu->GetObject<MobilityModel> ();
	    Vector pos_obu = obuMobility->GetPosition ();
	    std::cout << "(" << CalculateDistance (pos_obu, pos_rsu) << ")";
#endif // Print_Distance_OBU_RSU
	  }
	std::cout << endl;
#endif
#if 0
	std::cout << endl;
	std::cout << "req:";
	for (uint32_t req : m_fogsReqs[i])
	  {
	    std::cout << " " << req;
	  }
	std::cout << endl;
	std::cout << "cache:";
	for (uint32_t cache : m_fogsCaches[i])
	  {
	    std::cout << " " << cache;
	  }
	std::cout << endl;
#endif
      }
    std::cout << endl;
  }
}

void
VanetCsVfcExperiment::FacsFunc ()
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_LOG_UNCOND("Satisfied requests:" << m_pktsStats.GetSatisfiedReqsOneSchedule());
  NS_LOG_UNCOND("Count of V2V recv :" << m_requestStatsMap[m_currentBroadcastId].GetV2vRxPkts());
  NS_LOG_UNCOND("Count of V2F recv :" << m_requestStatsMap[m_currentBroadcastId].GetV2fRxPkts());
  NS_LOG_UNCOND("Requests remain :" << m_pktsStats.GetSubmittedReqs() << " - " << m_pktsStats.GetSatisfiedReqs()
		<< " = " << (m_pktsStats.GetSubmittedReqs() - m_pktsStats.GetSatisfiedReqs()));

  m_statsEveryPeriod[m_currentBroadcastId][1] = m_pktsStats.GetSatisfiedReqsOneSchedule();
  m_statsEveryPeriod[m_currentBroadcastId][2] = m_requestStatsMap[m_currentBroadcastId].GetV2vRxPkts()
      + m_requestStatsMap[m_currentBroadcastId].GetV2fRxPkts();

  m_pktsStats.ResetSatisfiedReqsOneSchedule();

  std::set<uint32_t> vehs;
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      if (m_vehsEnter[i]) vehs.insert(i);
    }
  if (vehs.size() == 0)
    {
      return;
    }

  // If all data requested are satisfied, return.
  bool flag = true;
  for (uint32_t veh : vehs)
    {
      if (!m_vehsReqs[veh].empty())
	{
	  flag = false;
	  break;
	}
    }
  if (flag) return;

  UpdateRemainingCommuTime ();

  mwSize dims[2] = {m_nObuNodes, m_globalDbSize};
  mwArray mwVehsCaches		(2, dims, mxUINT8_CLASS);
  mwArray mwVehsReqs		(2, dims, mxUINT8_CLASS);
  mwArray mwC2vRemainingTime	(1, m_nObuNodes, mxDOUBLE_CLASS);
  mwArray mwV2vRemainingTime	(m_nObuNodes, m_nObuNodes, mxDOUBLE_CLASS);
  mwArray mwV2fRemainingTime	(m_nRsuNodes, m_nObuNodes, mxDOUBLE_CLASS);
  mwArray mwDataRate		(1, 3, mxDOUBLE_CLASS);
  mwArray mwPktSize		(1, 1, mxDOUBLE_CLASS);

  for (uint32_t obuIdx = 0; obuIdx < m_nObuNodes; obuIdx++)
    {
      for (uint32_t i = 0; i < m_globalDbSize; i++)
	{
	  if (m_vehsCaches[obuIdx].count(i))
	    {
	      mwVehsCaches(obuIdx + 1, i + 1) = 1;
	    }
	  if (m_vehsReqs[obuIdx].count(i))
	    {
	      mwVehsReqs(obuIdx + 1, i + 1) = 1;
	    }
	}
    }

//  std::ofstream ofs;
//  std::ostringstream oss;
//  oss << m_cwd << "/request.txt";
//  ofs.open(oss.str(), ios::out);
//  for (uint32_t obuIdx = 0; obuIdx < m_nObuNodes; obuIdx++)
//    {
//      for (uint32_t i = 0; i < m_globalDbSize; i++)
//	{
//	  ofs << " " << (uint32_t)mwVehsReqs(obuIdx + 1, i + 1);
//	}
//      ofs << endl;
//    }
//  ofs.close();
//
//  oss.str("");
//  oss << m_cwd << "/cache.txt";
//  ofs.open(oss.str(), ios::out);
//  for (uint32_t obuIdx = 0; obuIdx < m_nObuNodes; obuIdx++)
//    {
//      for (uint32_t i = 0; i < m_globalDbSize; i++)
//	{
//	  ofs << " " << (uint32_t)mwVehsCaches(obuIdx + 1, i + 1);
//	}
//      ofs << endl;
//    }
//  ofs.close();

  for (uint32_t j = 1; j <= m_nObuNodes; j++)
    {
      mwC2vRemainingTime(1, j) = m_c2vRemainingTime[j - 1];
    }

  for (uint32_t i = 1; i <= m_nObuNodes; i++)
    {
      for (uint32_t j = i; j <= m_nObuNodes; j++)
	{
	  mwV2vRemainingTime(i, j) = m_v2vRemainingTime[i - 1][j - 1];
	  mwV2vRemainingTime(j, i) = m_v2vRemainingTime[i - 1][j - 1];
	}
    }

  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      for (uint32_t vehIdx : m_fogCluster[i])
	{
	  mwV2fRemainingTime(i + 1, vehIdx + 1) = m_v2fRemainingTime[i][vehIdx];
	}
    }

  mwDataRate(1, 1) = Rate_C2V;
  mwDataRate(1, 2) = Rate_V2F;
  mwDataRate(1, 3) = Rate_V2V;

  mwPktSize(1, 1) = m_dataSize;

  mwArray mwFacsResult1(1, 1, mxUINT32_CLASS);
  mwArray mwFacsResult2(mxINT32_CLASS);
  mwArray mwFacsResult3(1, 1, mxDOUBLE_CLASS);

  cout << "-----------------------------------------------------------------------------------" << endl;
  cout << "enter FACS at " << Now().GetSeconds() << "s" << endl;

  FACS(3, // number of result parameters
       mwFacsResult1, // result1: rows of result2
       mwFacsResult2, // result2: scheduling strategy
       mwFacsResult3, // result3: next schedule delay
       mwVehsCaches,
       mwVehsReqs,
       mwC2vRemainingTime,
       mwV2vRemainingTime,
       mwV2fRemainingTime,
       mwDataRate,
       mwPktSize);

  cout << "exit FACS" << endl;

  uint32_t rows = mwFacsResult1.Get(1, 1);
  cout << "count of result:" << rows << endl;
  if (rows == 0)
    {
      return;
    }

  double delay = (double)mwFacsResult3(1, 1);
  cout << "next schedule delay:" << delay << endl;
  double nextSchedulDelay = (1 + DelayDelta) * delay;

  // next schedule
  Simulator::Schedule(Seconds(nextSchedulDelay), &VanetCsVfcExperiment::DoScheduling, this);

  uint32_t nClique = 1;
  for (uint32_t n = 1; n <= nClique; n++)
    {
      m_currentBroadcastId++;

      // foreach 1st
      std::map<uint32_t, std::map<uint32_t, std::vector<TaskItem>>> v2vTasks;
      std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, std::vector<TaskItem>>>> v2fTasks; // key1=srcNodeIdx,key2=nextNodeIdx,key3=dataReqed

      for (uint32_t i = 1; i <= rows; i++)
      	{
      	  FacsResultItem rtItem;
	  rtItem.vehIdx = (uint32_t)mwFacsResult2(i, 2);
	  rtItem.dataReqed = (uint32_t)mwFacsResult2(i, 3);
	  rtItem.coVehIdx = (int)mwFacsResult2(i, 4);
	  rtItem.dataMerged = (int)mwFacsResult2(i, 5);
	  rtItem.transMode = (int)mwFacsResult2(i, 6);
	  m_FacsResultMap[m_currentBroadcastId][rtItem.vehIdx] = rtItem;

	  if (rtItem.transMode == 1)
	    {
	      cout << "test 2-1" << endl;
	      m_isEncoded[m_currentBroadcastId] = true;
	      TaskItem v2vTaskItem;
	      v2vTaskItem.srcNodeIdx = rtItem.coVehIdx;
	      v2vTaskItem.destNodeIdx = rtItem.vehIdx;
	      v2vTaskItem.dataReqed = rtItem.dataReqed;
	      v2vTasks[rtItem.coVehIdx][rtItem.vehIdx].push_back(v2vTaskItem);

//	      NS_LOG_DEBUG("send v2v: V" << rtItem.coVehIdx << " --> V" << rtItem.vehIdx);
	    }
	  else if(rtItem.transMode == 2)
	    {
	      m_isEncoded[m_currentBroadcastId] = true;
	      if (m_vehIdx2FogIdxMap.count(rtItem.vehIdx)
		  && m_vehIdx2FogIdxMap.count(rtItem.coVehIdx))
		{
		  uint32_t fogIdx1 = m_vehIdx2FogIdxMap.at(rtItem.vehIdx);
		  uint32_t fogIdx2 = m_vehIdx2FogIdxMap.at(rtItem.coVehIdx);

		  TaskItem v2fTaskItem;
		  v2fTaskItem.srcNodeIdx = rtItem.coVehIdx;
		  v2fTaskItem.destNodeIdx = rtItem.vehIdx;
		  v2fTaskItem.dataReqed = rtItem.dataReqed;
		  v2fTaskItem.nextFogIdx = fogIdx1;
		  v2fTaskItem.nextAction = (fogIdx1 == fogIdx2) ? 1 : 2;
		  v2fTasks[rtItem.coVehIdx][fogIdx2][rtItem.dataReqed].push_back(v2fTaskItem);

//		  NS_LOG_DEBUG("send v2f: V" << rtItem.coVehIdx << " --> F" << m_rsuNodes.Get(fogIdx2)->GetId()
//		      << " --> F" << m_rsuNodes.Get(fogIdx1)->GetId()
//		      << " --> V" << rtItem.vehIdx);
		  NS_LOG_DEBUG("send v2f: V" << rtItem.coVehIdx << " --> F" << fogIdx2
		      << " --> F" << fogIdx1 << " --> V" << rtItem.vehIdx);
		}
	    }

	  std::cout.setf(std::ios::left);
	  std::cout << "vehIdx: " << std::setw(5) << rtItem.vehIdx
		    << "vehFogIdx: " << std::setw(5) << (m_vehIdx2FogIdxMap.count(rtItem.vehIdx) ? (int)m_vehIdx2FogIdxMap[rtItem.vehIdx] : -1)
		    << "dataReqed: " << std::setw(5) << rtItem.dataReqed
		    << "coVehIdx: " << std::setw(5) << rtItem.coVehIdx
		    << "coVehFogIdx: " << std::setw(5) << (m_vehIdx2FogIdxMap.count(rtItem.coVehIdx) ? (int)m_vehIdx2FogIdxMap[rtItem.coVehIdx] : -1)
		    << "dataMerged: " << std::setw(5) << rtItem.dataMerged
		    << "transMode: " << std::setw(5) << rtItem.transMode << std::endl;

      	  m_dataToSatisfy[m_currentBroadcastId][rtItem.vehIdx] = rtItem.dataReqed;

//        FACSResultItem item;
//	  item.vehIdx = vehIdx;
//	  item.dataReqed = dataReqed;
//	  item.coVehIdx = coVehIdx1;
//	  item.dataMerged = dataMerged1;
//	  item.transMode = transMode;
//	  m_FACSResultMap[currentBroadcastId][vehIdx] = item;

	  m_vehsToSatisfy[m_currentBroadcastId].insert(rtItem.vehIdx);
	  if (std::count(m_dataToBroadcast[m_currentBroadcastId].begin(), m_dataToBroadcast[m_currentBroadcastId].end(), rtItem.dataReqed) == 0)
	    {
	      m_dataToBroadcast[m_currentBroadcastId].push_back(rtItem.dataReqed);
	    }
      	}

      // broadcast
      m_pktsStats.IncBroadcastPkts();

      Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
      dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
      dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//      InetSocketAddress local = InetSocketAddress (m_remoteHostAddr, m_dlPort);
//      dataTxApp->SetAttribute ("Local", AddressValue (local));
      InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.2.255.255"), m_dlPort);
      dataTxApp->SetAttribute ("Remote", AddressValue (remote));
      std::ostringstream oss;
      oss << Rate_C2V << "Mb/s";
      dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
      dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
      dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
      dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

      using vanet::PacketHeader;
      PacketHeader header;
      header.SetType(PacketHeader::MessageType::DATA_C2V);
      header.SetDataPktId(m_dataPktId++);
      header.SetDataSize(m_dataSize);
      header.SetBroadcastId(m_currentBroadcastId);
      dataTxApp->SetHeader(header);

      using vanet::PacketTagC2v;
      PacketTagC2v *pktTagC2v = new PacketTagC2v();
      std::vector<uint32_t> reqsIds;
      for (uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
	{
	  reqsIds.push_back(data);
	}
      pktTagC2v->SetReqsIds(reqsIds);
      dataTxApp->SetPacketTag(pktTagC2v);
      m_appContainer.Add(dataTxApp);

      m_remoteHost->AddApplication(dataTxApp);

      dataTxApp->SetStartTime(Seconds(0));

      // v2v
      std::map<uint32_t, std::map<uint32_t, std::vector<TaskItem>>>::iterator it1 = v2vTasks.begin();
      for (; it1 != v2vTasks.end(); it1++)
	{
	  uint32_t coVehIdx = it1->first;
	  std::map<uint32_t, std::vector<TaskItem>>::iterator it1_1 = it1->second.begin();
	  for (; it1_1 != it1->second.end(); it1_1++)
	    {
	      uint32_t vehIdx = it1_1->first;
	      for (TaskItem taskItem : it1_1->second)
		{
		  NS_ASSERT(m_vehNeighbors[vehIdx][coVehIdx] && m_vehNeighbors[coVehIdx][vehIdx]);

		  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
		  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
		  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//		  InetSocketAddress local = InetSocketAddress (m_obuInterfacesTx.GetAddress(vehIdx), m_v2VPort);
//		  onOffApp->SetAttribute ("Local", AddressValue (local));
		  InetSocketAddress remote = InetSocketAddress (m_obuInterfacesRx.GetAddress(vehIdx), m_v2VPort);
		  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
		  std::ostringstream oss;
		  oss << Rate_V2V << "Mb/s";
		  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
		  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
		  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
		  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

		  using vanet::PacketHeader;
		  PacketHeader header;
		  header.SetType(PacketHeader::MessageType::DATA_V2V);
		  header.SetDataPktId(m_dataPktId++);
		  header.SetDataSize(m_dataSize);
		  header.SetBroadcastId(m_currentBroadcastId);
		  dataTxApp->SetHeader(header);

		  using vanet::PacketTagV2v;
		  PacketTagV2v *pktTagV2v = new PacketTagV2v();
		  pktTagV2v->SetSrcNodeId(coVehIdx);
		  std::vector<uint32_t> dataIdxs;
		  dataIdxs.push_back(taskItem.dataReqed);
		  pktTagV2v->SetDataIdxs(dataIdxs);
		  dataTxApp->SetPacketTag(pktTagV2v);
		  m_appContainer.Add(dataTxApp);

		  m_obuNodes.Get (coVehIdx)->AddApplication(dataTxApp);
		  dataTxApp->SetStartTime(Seconds(0));

		  NS_LOG_DEBUG("send v2v: V" << taskItem.srcNodeIdx << " --> V" << taskItem.destNodeIdx);

		  PacketStats reqStats;
		  if (m_requestStatsMap.count(m_currentBroadcastId) != 0)
		    {
		      reqStats = m_requestStatsMap.at(m_currentBroadcastId);
		    }
		  reqStats.IncV2vTxPkts();
		  m_requestStatsMap[m_currentBroadcastId] = reqStats;

		  m_pktsStats.IncV2vTxPkts();
		}
	    }
	}

      // v2f
      std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, std::vector<TaskItem>>>>::iterator it2 = v2fTasks.begin();
      for (; it2 != v2fTasks.end(); it2++)
	{
	  uint32_t coVehIdx = it2->first;
	  std::map<uint32_t, std::map<uint32_t, std::vector<TaskItem>>>::iterator it2_1 = it2->second.begin();
	  for (; it2_1 != it2->second.end(); it2_1++)
	    {
	      uint32_t fogIdx = it2_1->first;
	      std::map<uint32_t, std::vector<TaskItem>>::iterator it2_1_1 = it2_1->second.begin();
	      for (; it2_1_1 != it2_1->second.end(); it2_1_1++)
		{
		  uint32_t dataReqed = it2_1_1->first;
		  std::vector<uint32_t> nextObusIdx;
		  std::vector<uint32_t> nextRsusIdx;
		  std::map<uint32_t, std::vector<uint32_t>> destObusIdx;
		  for (TaskItem taskItem : it2_1_1->second)
		    {
		      if (taskItem.nextAction == 1)
			{
			  nextObusIdx.push_back(taskItem.destNodeIdx);
			}
		      else if (taskItem.nextAction == 2)
			{
			  nextRsusIdx.push_back(taskItem.nextFogIdx);
			  destObusIdx[taskItem.nextFogIdx].push_back(taskItem.destNodeIdx);
			}

		      // v2f statistic
		      PacketStats reqStats;
		      if (m_requestStatsMap.count(m_currentBroadcastId) != 0)
			{
			  reqStats = m_requestStatsMap.at(m_currentBroadcastId);
			}
		      reqStats.IncV2fTxPkts();
		      m_requestStatsMap[m_currentBroadcastId] = reqStats;

		      m_pktsStats.IncV2fTxPkts();
		    }

//		  cout << "nextObuIdx:";
//		  for (uint32_t test : nextObusIdx)
//		    {
//		      cout << " " << test;
//		    }
//		  cout << endl;
//
//		  cout << "nextRsusIdx:";
//		  for (uint32_t test : nextRsusIdx)
//		    {
//		      cout << " " << test;
//		    }
//		  cout << endl;
//
//		  cout << "destObusIdx:";
//		  std::map<uint32_t, std::vector<uint32_t>>::iterator destObusIdxIt = destObusIdx.begin();
//		  for (; destObusIdxIt != destObusIdx.end(); ++destObusIdxIt)
//		    {
//		      cout << destObusIdxIt->first << "=";
//		      for (uint32_t obuIdx : destObusIdxIt->second)
//			{
//			  cout << " " << obuIdx;
//			}
//		      cout << endl;
//		    }
//		  cout << endl;

		  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
		  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
		  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
		  InetSocketAddress remote = InetSocketAddress (m_rsu80211pInterfacesRx.GetAddress(fogIdx), m_v2IPort);
		  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
		  std::ostringstream oss;
		  oss << Rate_V2F << "Mb/s";
		  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
		  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
		  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
		  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

		  using vanet::PacketHeader;
		  PacketHeader header;
		  header.SetType(PacketHeader::MessageType::DATA_V2F);
		  header.SetDataPktId(m_dataPktId++);
		  header.SetDataSize(m_dataSize);
		  header.SetBroadcastId(m_currentBroadcastId);
		  dataTxApp->SetHeader(header);

		  using vanet::PacketTagV2f;
		  PacketTagV2f *pktTagV2f = new PacketTagV2f();
		  pktTagV2f->SetNextObusIdx(nextObusIdx);
		  pktTagV2f->SetNextRsusIdx(nextRsusIdx);
		  pktTagV2f->SetDestObusIdx(destObusIdx);
		  std::vector<uint32_t> dataIdxs;
		  dataIdxs.push_back(dataReqed);
		  pktTagV2f->SetDataIdxs(dataIdxs);
		  dataTxApp->SetPacketTag(pktTagV2f);
		  m_appContainer.Add(dataTxApp);

		  m_obuNodes.Get (coVehIdx)->AddApplication(dataTxApp);
		  dataTxApp->SetStartTime(Seconds(0));
		}

	    }
	}

      std::cout << "bID " << m_currentBroadcastId << ":";
      for(uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
	{
	  std::cout << " " << data;
	}
      std::cout << std::endl;

      uint32_t v2vplusv2f = m_requestStatsMap[m_currentBroadcastId].GetV2vTxPkts() + m_requestStatsMap[m_currentBroadcastId].GetV2fTxPkts();
      NS_LOG_UNCOND("All of requests :" << m_vehsToSatisfy[m_currentBroadcastId].size()
		    << " + " << v2vplusv2f << " = " << (m_vehsToSatisfy[m_currentBroadcastId].size() + v2vplusv2f));

      NS_LOG_UNCOND("Count of V2V sent :" << m_requestStatsMap[m_currentBroadcastId].GetV2vTxPkts());
      NS_LOG_UNCOND("Count of V2F sent :" << m_requestStatsMap[m_currentBroadcastId].GetV2fTxPkts());

      m_statsEveryPeriod[m_currentBroadcastId][0] = m_vehsToSatisfy[m_currentBroadcastId].size() + v2vplusv2f;
    }
}

void
VanetCsVfcExperiment::Decode (bool isEncoded, uint32_t broadcastId)
{


}

void VanetCsVfcExperiment::DoStats (uint32_t obuIdx, uint32_t dataIdx)
{
  NS_LOG_FUNCTION_NOARGS ();

  std::map<uint32_t, RequestStatus>::iterator it = m_vehsReqsStatus[obuIdx].find(dataIdx);
  NS_ASSERT_MSG(it != m_vehsReqsStatus[obuIdx].end(), "iter == end()");
  if (it->second.completed) return;
  it->second.completed = true;
  it->second.satisfiedTime = Now().GetDouble();

  m_requestStatsEachVeh[obuIdx].IncSatisfiedReqs();
  m_requestStatsEachVeh[obuIdx].IncCumulativeDelay(it->second.satisfiedTime - it->second.submitTime);

  m_pktsStats.IncSatisfiedReqs();
  m_pktsStats.IncCumulativeDelay(it->second.satisfiedTime - it->second.submitTime);

  m_pktsStats.IncSatisfiedReqsOneSchedule();
}

void
VanetCsVfcExperiment::NcbFunc ()
{
  NS_LOG_FUNCTION_NOARGS ();

  std::set<uint32_t> vehs;
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      if (m_vehsEnter[i]) vehs.insert(i);
    }
  if (vehs.size() == 0)
    {
      return;
    }

  // If all data requested are satisfied, return.
  bool flag = true;
  for (uint32_t veh : vehs)
    {
      if (!m_vehsReqs[veh].empty())
	{
	  flag = false;
	  break;
	}
    }
  if (flag) return;

  UpdateRemainingCommuTime ();

  mwSize dims[2] = {m_nObuNodes, m_globalDbSize};
  mwArray mwVehsCaches		(2, dims, mxUINT8_CLASS);
  mwArray mwVehsReqs		(2, dims, mxUINT8_CLASS);
  mwArray mwC2vRemainingTime	(1, m_nObuNodes, mxDOUBLE_CLASS);
  mwArray mwV2fRemainingTime	(m_nRsuNodes, m_nObuNodes, mxDOUBLE_CLASS);
  mwArray mwDataRate		(1, 3, mxDOUBLE_CLASS);
  mwArray mwPktSize		(1, 1, mxDOUBLE_CLASS);

  for (uint32_t obuIdx = 0; obuIdx < m_nObuNodes; obuIdx++)
    {
      for (uint32_t i = 0; i < m_globalDbSize; i++)
	{
	  if (m_vehsCaches[obuIdx].count(i))
	    {
	      mwVehsCaches(obuIdx + 1, i + 1) = 1;
	    }
	  if (m_vehsReqs[obuIdx].count(i))
	    {
	      mwVehsReqs(obuIdx + 1, i + 1) = 1;
	    }
	}
    }

  for (uint32_t j = 1; j <= m_nObuNodes; j++)
    {
      mwC2vRemainingTime(1, j) = m_c2vRemainingTime[j - 1];
    }

  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      for (uint32_t vehIdx : m_fogCluster[i])
	{
	  mwV2fRemainingTime(i + 1, vehIdx + 1) = m_v2fRemainingTime[i][vehIdx];
	}
    }

  mwDataRate(1, 1) = Rate_C2V;
  mwDataRate(1, 2) = Rate_V2F;
  mwDataRate(1, 3) = Rate_V2V;

  mwPktSize(1, 1) = m_dataSize;

  mwArray mwNcbResult1(1, 1, mxUINT32_CLASS);	// result1: number of data to broadcast
  mwArray mwNcbResult2(1, 2, mxUINT32_CLASS);	// result2: data to broadcast
  mwArray mwNcbResult3(1, 1, mxUINT32_CLASS);	// result3: rows of result4
  mwArray mwNcbResult4(mxINT32_CLASS);		// result4: scheduling strategy
  mwArray mwNcbResult5(1, 1, mxDOUBLE_CLASS);	// result5: next schedule delay

  cout << "-----------------------------------------------------------------------------------" << endl;
  cout << "enter NCB at " << Now().GetSeconds() << "s" << endl;

  NCB(5, // number of result parameters
       mwNcbResult1,
       mwNcbResult2,
       mwNcbResult3,
       mwNcbResult4,
       mwNcbResult5,
       mwVehsCaches,
       mwVehsReqs,
       mwC2vRemainingTime,
       mwV2fRemainingTime,
       mwDataRate,
       mwPktSize);

  cout << "exit NCB" << endl;

  uint32_t nBroadcastData = mwNcbResult1(1, 1);
  cout << "count of data:" << nBroadcastData << endl;

  cout << "data to broadcast:";
  for (uint32_t i = 0; i < nBroadcastData; ++i)
    {
      m_dataToBroadcast[m_currentBroadcastId].push_back(mwNcbResult2(1, i + 1));
      cout << " " << mwNcbResult2(1, i + 1);
    }
  cout << endl;

  uint32_t rows = mwNcbResult3(1, 1);
  cout << "count of result:" << rows << endl;
  if (rows == 0)
    {
      return;
    }

  double delay = (double)mwNcbResult5(1, 1);
  cout << "next schedule delay:" << delay << endl;
  double nextSchedulDelay = (1 + DelayDelta) * delay;

  // next schedule
  Simulator::Schedule(Seconds(nextSchedulDelay), &VanetCsVfcExperiment::DoScheduling, this);

  m_currentBroadcastId++;

  if (nBroadcastData > 1)
    {
      m_isEncoded[m_currentBroadcastId] = true;
    }
  // foreach 1st
  std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, std::vector<TaskItem>>>> v2fTasks; // key1=srcNodeIdx,key2=nextNodeIdx,key3=dataReqed

  for (uint32_t i = 1; i <= rows; i++)
    {
      NcbResultItem rtItem;
      rtItem.vehIdx = (uint32_t)mwNcbResult4(i, 2);
      rtItem.dataReqed1 = (uint32_t)mwNcbResult4(i, 3);
      rtItem.dataReqed2 = (int)mwNcbResult4(i, 4);
      rtItem.coVehIdx = (int)mwNcbResult4(i, 5);
      rtItem.fogIdx = (int)mwNcbResult4(i, 6);
      m_NcbResultMap[m_currentBroadcastId][rtItem.vehIdx] = rtItem;

      if(rtItem.dataReqed2 != -1)
	{
	  TaskItem v2fTaskItem;
	  v2fTaskItem.srcNodeIdx = rtItem.coVehIdx;
	  v2fTaskItem.destNodeIdx = rtItem.vehIdx;
	  v2fTaskItem.dataReqed = rtItem.dataReqed2;
	  v2fTaskItem.nextFogIdx = rtItem.fogIdx;
	  v2fTaskItem.nextAction = 1;
	  v2fTasks[rtItem.coVehIdx][rtItem.fogIdx][rtItem.dataReqed2].push_back(v2fTaskItem);

	  NS_LOG_DEBUG("send v2f: V" << rtItem.coVehIdx << " --> F" << rtItem.fogIdx
	      << " --> V" << rtItem.vehIdx);
	}

      std::cout.setf(std::ios::left);
      std::cout << "vehIdx: " << std::setw(5) << rtItem.vehIdx
		<< "dataReqed1: " << std::setw(5) << rtItem.dataReqed1
		<< "dataReqed2: " << std::setw(5) << rtItem.dataReqed2
		<< "coVehIdx: " << std::setw(5) << rtItem.coVehIdx
		<< "fogIdx: " << std::setw(5) << rtItem.fogIdx << std::endl;

      m_dataToSatisfy[m_currentBroadcastId][rtItem.vehIdx] = rtItem.dataReqed1;

      m_vehsToSatisfy[m_currentBroadcastId].insert(rtItem.vehIdx);
      if (std::count(m_dataToBroadcast[m_currentBroadcastId].begin(), m_dataToBroadcast[m_currentBroadcastId].end(), rtItem.dataReqed1) == 0)
	{
	  m_dataToBroadcast[m_currentBroadcastId].push_back(rtItem.dataReqed1);
	}
    }

  // broadcast
  m_pktsStats.IncBroadcastPkts();

  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.2.255.255"), m_dlPort);
  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
  std::ostringstream oss;
  oss << Rate_C2V << "Mb/s";
  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

  using vanet::PacketHeader;
  PacketHeader header;
  header.SetType(PacketHeader::MessageType::DATA_C2V);
  header.SetDataPktId(m_dataPktId++);
  header.SetDataSize(m_dataSize);
  header.SetBroadcastId(m_currentBroadcastId);
  dataTxApp->SetHeader(header);

  using vanet::PacketTagC2v;
  PacketTagC2v *pktTagC2v = new PacketTagC2v();
  std::vector<uint32_t> reqsIds;
  for (uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
    {
      reqsIds.push_back(data);
    }
  pktTagC2v->SetReqsIds(reqsIds);
  dataTxApp->SetPacketTag(pktTagC2v);
  m_appContainer.Add(dataTxApp);

  m_remoteHost->AddApplication(dataTxApp);

  dataTxApp->SetStartTime(Seconds(0));

  // v2f
  std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, std::vector<TaskItem>>>>::iterator it2 = v2fTasks.begin();
  for (; it2 != v2fTasks.end(); it2++)
    {
      uint32_t coVehIdx = it2->first;
      std::map<uint32_t, std::map<uint32_t, std::vector<TaskItem>>>::iterator it2_1 = it2->second.begin();
      for (; it2_1 != it2->second.end(); it2_1++)
	{
	  uint32_t fogIdx = it2_1->first;
	  std::map<uint32_t, std::vector<TaskItem>>::iterator it2_1_1 = it2_1->second.begin();
	  for (; it2_1_1 != it2_1->second.end(); it2_1_1++)
	    {
	      uint32_t dataReqed = it2_1_1->first;
	      std::vector<uint32_t> nextObusIdx;
	      for (TaskItem taskItem : it2_1_1->second)
		{
		  NS_ASSERT(taskItem.nextAction == 1);
		  nextObusIdx.push_back(taskItem.destNodeIdx);

		  // v2f statistic
		  PacketStats reqStats;
		  if (m_requestStatsMap.count(m_currentBroadcastId) != 0)
		    {
		      reqStats = m_requestStatsMap.at(m_currentBroadcastId);
		    }
		  reqStats.IncV2fTxPkts();
		  m_requestStatsMap[m_currentBroadcastId] = reqStats;

		  m_pktsStats.IncV2fTxPkts();
		}

//		  cout << "nextObusIdx:";
//		  for (uint32_t test : nextObusIdx)
//		    {
//		      cout << " " << test;
//		    }
//		  cout << endl;

	      Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
	      dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
	      dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	      InetSocketAddress remote = InetSocketAddress (m_rsu80211pInterfacesRx.GetAddress(fogIdx), m_v2IPort);
	      dataTxApp->SetAttribute ("Remote", AddressValue (remote));
	      std::ostringstream oss;
	      oss << Rate_V2F << "Mb/s";
	      dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
	      dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
	      dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
	      dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

	      using vanet::PacketHeader;
	      PacketHeader header;
	      header.SetType(PacketHeader::MessageType::DATA_V2F);
	      header.SetDataPktId(m_dataPktId++);
	      header.SetDataSize(m_dataSize);
	      header.SetBroadcastId(m_currentBroadcastId);
	      dataTxApp->SetHeader(header);

	      using vanet::PacketTagV2f;
	      PacketTagV2f *pktTagV2f = new PacketTagV2f();
	      pktTagV2f->SetNextObusIdx(nextObusIdx);
//	      pktTagV2f->SetNextRsusIdx(nextRsusIdx);
//	      pktTagV2f->SetDestObusIdx(destObusIdx);
	      std::vector<uint32_t> dataIdxs;
	      dataIdxs.push_back(dataReqed);
	      pktTagV2f->SetDataIdxs(dataIdxs);
	      dataTxApp->SetPacketTag(pktTagV2f);
	      m_appContainer.Add(dataTxApp);

	      m_obuNodes.Get (coVehIdx)->AddApplication(dataTxApp);
	      dataTxApp->SetStartTime(Seconds(0));
	    }
	}
    }

  std::cout << "bID " << m_currentBroadcastId << ":";
  for(uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
    {
      std::cout << " " << data;
    }
  std::cout << std::endl;
}

void
VanetCsVfcExperiment::IsxdFunc ()
{
  NS_LOG_FUNCTION_NOARGS ();

  std::set<uint32_t> vehs;
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      if (m_vehsEnter[i]) vehs.insert(i);
    }
  if (vehs.size() == 0)
    {
      return;
    }

  // If all data requested are satisfied, return.
  bool flag = true;
  for (uint32_t veh : vehs)
    {
      if (!m_vehsReqs[veh].empty())
	{
	  flag = false;
	  break;
	}
    }
  if (flag) return;

  UpdateRemainingCommuTime ();

  mwSize dims[2] = {m_nObuNodes, m_globalDbSize};
  mwArray mwVehsCaches		(2, dims, mxUINT8_CLASS);
  mwArray mwVehsReqs		(2, dims, mxUINT8_CLASS);
  mwArray mwC2vRemainingTime	(1, m_nObuNodes, mxDOUBLE_CLASS);
  mwArray mwV2fRemainingTime	(m_nRsuNodes, m_nObuNodes, mxDOUBLE_CLASS);
  mwArray mwDataRate		(1, 3, mxDOUBLE_CLASS);
  mwArray mwPktSize     	(1, m_globalDbSize, mxDOUBLE_CLASS);

  for (uint32_t obuIdx = 0; obuIdx < m_nObuNodes; obuIdx++)
    {
      for (uint32_t i = 0; i < m_globalDbSize; i++)
	{
	  if (m_vehsCaches[obuIdx].count(i))
	    {
	      mwVehsCaches(obuIdx + 1, i + 1) = 1;
	    }
	  if (m_vehsReqs[obuIdx].count(i))
	    {
	      mwVehsReqs(obuIdx + 1, i + 1) = 1;
	    }
	}
    }

  for (uint32_t j = 1; j <= m_nObuNodes; j++)
    {
      mwC2vRemainingTime(1, j) = m_c2vRemainingTime[j - 1];
    }

  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      for (uint32_t vehIdx : m_fogCluster[i])
	{
	  mwV2fRemainingTime(i + 1, vehIdx + 1) = m_v2fRemainingTime[i][vehIdx];
	}
    }

  mwDataRate(1, 1) = Rate_C2V;
  mwDataRate(1, 2) = Rate_V2F;
  mwDataRate(1, 3) = Rate_V2V;

  for (uint32_t i = 0; i < m_globalDbSize; i++)
    {
      mwPktSize(1, i + 1) = m_dataSize;
    }

  mwArray mwIsxdResult1(1, 1, mxUINT32_CLASS);	// result1: rows of result2
  mwArray mwIsxdResult2(mxINT32_CLASS);		// result2: scheduling strategy
  mwArray mwIsxdResult3(1, 1, mxDOUBLE_CLASS);	// result3: next schedule delay

  cout << "-----------------------------------------------------------------------------------" << endl;
  cout << "enter ISXD at " << Now().GetSeconds() << "s" << endl;

  ISXD(3, // number of result parameters
       mwIsxdResult1,
       mwIsxdResult2,
       mwIsxdResult3,
       mwVehsCaches,
       mwVehsReqs,
       mwC2vRemainingTime,
       mwV2fRemainingTime,
       mwDataRate,
       mwPktSize);

  cout << "exit ISXD" << endl;

  uint32_t rows = mwIsxdResult1(1, 1);
  cout << "count of result:" << rows << endl;
  if (rows == 0)
    {
      return;
    }

  double delay = (double)mwIsxdResult3(1, 1);
  cout << "next schedule delay:" << delay << endl;
  double nextSchedulDelay = (1 + DelayDelta) * delay;

  // next schedule
  Simulator::Schedule(Seconds(nextSchedulDelay), &VanetCsVfcExperiment::DoScheduling, this);

  m_currentBroadcastId++;

  // foreach 1st
  std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, std::vector<TaskItem>>>> v2fTasks; // key1=srcNodeIdx,key2=nextNodeIdx,key3=dataReqed

  for (uint32_t i = 1; i <= rows; i++)
    {
      IsxdResultItem rtItem;
      rtItem.vehIdx = (uint32_t)mwIsxdResult2(i, 2);
      rtItem.dataReqed = (uint32_t)mwIsxdResult2(i, 3);
      m_IsxdResultMap[m_currentBroadcastId][rtItem.vehIdx] = rtItem;

      std::cout.setf(std::ios::left);
      std::cout << "vehIdx: " << std::setw(5) << rtItem.vehIdx
		<< "dataReqed: " << std::setw(5) << rtItem.dataReqed << std::endl;

      m_dataToSatisfy[m_currentBroadcastId][rtItem.vehIdx] = rtItem.dataReqed;

      m_vehsToSatisfy[m_currentBroadcastId].insert(rtItem.vehIdx);
      if (std::count(m_dataToBroadcast[m_currentBroadcastId].begin(), m_dataToBroadcast[m_currentBroadcastId].end(), rtItem.dataReqed) == 0)
	{
	  m_dataToBroadcast[m_currentBroadcastId].push_back(rtItem.dataReqed);
	}
    }

  // broadcast
  m_pktsStats.IncBroadcastPkts();

  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.2.255.255"), m_dlPort);
  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
  std::ostringstream oss;
  oss << Rate_C2V << "Mb/s";
  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

  using vanet::PacketHeader;
  PacketHeader header;
  header.SetType(PacketHeader::MessageType::DATA_C2V);
  header.SetDataPktId(m_dataPktId++);
  header.SetDataSize(m_dataSize);
  header.SetBroadcastId(m_currentBroadcastId);
  dataTxApp->SetHeader(header);

  using vanet::PacketTagC2v;
  PacketTagC2v *pktTagC2v = new PacketTagC2v();
  std::vector<uint32_t> reqsIds;
  for (uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
    {
      reqsIds.push_back(data);
    }
  pktTagC2v->SetReqsIds(reqsIds);
  dataTxApp->SetPacketTag(pktTagC2v);
  m_appContainer.Add(dataTxApp);

  m_remoteHost->AddApplication(dataTxApp);

  dataTxApp->SetStartTime(Seconds(0));

  std::cout << "bID " << m_currentBroadcastId << ":";
  for(uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
    {
      std::cout << " " << data;
    }
  std::cout << std::endl;
}

void
VanetCsVfcExperiment::NcbFunc1 ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_requestQueue.empty()) return;

//  for (std::string req : requestsToMarkWithBid[currentBroadcastId])
//    {
//      requestsToMarkGloabal[req] = false;
//    }
//  requestsToMarkWithBid[currentBroadcastId].clear();

  std::list<ReqQueueItem>::iterator iter_list = m_requestQueue.begin();
//  while (iter_list != requestQueue.end() && requestsToMarkGloabal[iter_list->name])
//    {
//      iter_list++;
//    }
  while (iter_list != m_requestQueue.end())
    {
      if (m_requestsToMarkGloabal[iter_list->name] || !m_vehIdx2FogIdxMap.count(iter_list->vehIndex))
	{
	  iter_list++;
	}
      else
	{
	  break;
	}
    }
  if (iter_list == m_requestQueue.end()) return;

  m_currentBroadcastId++;
  m_reqQueHead[m_currentBroadcastId] = *iter_list;

  uint32_t maxNum = 0;
  std::vector<uint32_t> dataSet;
  dataSet.push_back(m_reqQueHead[m_currentBroadcastId].reqDataIndex);
  m_dataToBroadcast[m_currentBroadcastId] = dataSet;

  std::set<uint32_t> vehIdxTraversed;

  std::list<ReqQueueItem>::iterator iter = m_requestQueue.begin();
  for (; iter != m_requestQueue.end(); iter++)
    {
      if (vehIdxTraversed.count(iter->vehIndex)) continue;
      if (m_requestsToMarkGloabal[iter->name]) continue;

      if (m_reqQueHead[m_currentBroadcastId].reqDataIndex == iter->reqDataIndex)
	{
	  maxNum += 1;
	  m_vehsToSatisfy[m_currentBroadcastId].insert(iter->vehIndex);
	  m_requestsToMarkWithBid[m_currentBroadcastId].insert(iter->name);

	  vehIdxTraversed.insert(iter->vehIndex);
	}
    }

//  std::set<uint32_t> caches;
//  if (!vehIdx2FogIdxMap.count(reqQueHead[currentBroadcastId].vehIndex))
//    {
//      caches = vehsCaches[reqQueHead[currentBroadcastId].vehIndex];
//    }
//  else
//    {
//      caches = fogsCaches[vehIdx2FogIdxMap.at(reqQueHead[currentBroadcastId].vehIndex)];
//    }

  std::set<uint32_t> caches;
  caches = m_fogsCaches[m_vehIdx2FogIdxMap.at(m_reqQueHead[m_currentBroadcastId].vehIndex)];

  uint32_t data2ToBroadcast;
  bool flag = false;
  for (uint32_t cache : caches)
    {
      uint32_t maxNumEncode = 0;
      std::set<uint32_t> vehsToSatisfyEncode;
      std::set<std::string> requestsToMarkTmpEncode;
      std::set<uint32_t> vehIdxTraversedEncode;

      iter = m_requestQueue.begin();
      for (; iter != m_requestQueue.end(); iter++)
	{
	  if (vehIdxTraversedEncode.count(iter->vehIndex)) continue;
	  if (m_requestsToMarkGloabal[iter->name]) continue;

	  std::set<uint32_t> caches2;
	  if (!m_vehIdx2FogIdxMap.count(iter->vehIndex))
	    {
//	      caches2 = vehsCaches[iter->vehIndex];
	      continue;
	    }
	  else
	    {
	      caches2 = m_fogsCaches[m_vehIdx2FogIdxMap.at(iter->vehIndex)];
	    }
	  if (m_reqQueHead[m_currentBroadcastId].reqDataIndex == iter->reqDataIndex && caches2.count(cache))
	    {
	      maxNumEncode += 1;
	      vehsToSatisfyEncode.insert(iter->vehIndex);
	      requestsToMarkTmpEncode.insert(iter->name);

	      vehIdxTraversedEncode.insert(iter->vehIndex);
	    }
	  else if (cache == iter->reqDataIndex && caches2.count(m_reqQueHead[m_currentBroadcastId].reqDataIndex))
	    {
	      maxNumEncode += 1;
	      vehsToSatisfyEncode.insert(iter->vehIndex);
	      requestsToMarkTmpEncode.insert(iter->name);

	      vehIdxTraversedEncode.insert(iter->vehIndex);
	    }
	}
      if (maxNumEncode > maxNum)
	{
	  flag = true;
//	  cout << Now().GetSeconds() << ", maxNumEncode:" << maxNumEncode << ", maxNum:" << maxNum << endl;
	  maxNum = maxNumEncode;
	  m_vehsToSatisfy[m_currentBroadcastId] = vehsToSatisfyEncode;
	  m_requestsToMarkWithBid[m_currentBroadcastId] = requestsToMarkTmpEncode;
	  data2ToBroadcast = cache;
	}
    }

  for (std::string name : m_requestsToMarkWithBid[m_currentBroadcastId])
    {
      m_requestsToMarkGloabal[name] = true;
    }
  if (flag)
    {
      m_dataToBroadcast[m_currentBroadcastId].push_back(data2ToBroadcast);
    }

  cout << Now().GetSeconds() << ", maxNum:" << maxNum;
  cout << ", dataToBroadcast" << "[" << m_currentBroadcastId << "]" << ":";
  for (uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
    {
      cout << " " << data;
    }
//  cout << ", vehsToSatisfy";
//  for (uint32_t veh : vehsToSatisfy)
//    {
//      cout << " " << veh;
//    }
  cout << endl;
  std::cout << "DbSize:" << m_globalDbSize << std::endl;

//  m_pktsStats.IncBroadcastPkts();
//
//  Ptr<UdpSender> sender = CreateObject<UdpSender>();
//  sender->SetNode(m_remoteHost);
//  sender->SetRemote(Ipv4Address ("10.2.255.255"), m_dlPort);
//  sender->SetDataSize(m_packetSize);
//  sender->Start();
//  using vanet::PacketHeader;
//  PacketHeader header;
//  header.SetType(PacketHeader::MessageType::DATA_C2V);
//  header.SetBroadcastId(m_currentBroadcastId);
//  sender->SetHeader(header);
//
//  using vanet::PacketTagC2v;
//  PacketTagC2v *pktTag = new PacketTagC2v();
//  std::vector<uint32_t> reqsIds;
//  for (uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
//    {
//      reqsIds.push_back(data);
//    }
//  pktTag->SetReqsIds(reqsIds);
//  sender->SetPacketTag(pktTag);
//
//  Simulator::ScheduleNow (&UdpSender::Send, sender);

  // broadcast
  m_pktsStats.IncBroadcastPkts();

  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//      InetSocketAddress local = InetSocketAddress (m_remoteHostAddr, m_dlPort);
//      dataTxApp->SetAttribute ("Local", AddressValue (local));
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("10.2.255.255"), m_dlPort);
  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
  std::ostringstream oss;
  oss << Rate_C2V << "Mb/s";
  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

  using vanet::PacketHeader;
  PacketHeader header;
  header.SetType(PacketHeader::MessageType::DATA_C2V);
  header.SetDataPktId(m_dataPktId++);
  header.SetDataSize(m_dataSize);
  header.SetBroadcastId(m_currentBroadcastId);
  dataTxApp->SetHeader(header);

  using vanet::PacketTagC2v;
  PacketTagC2v *pktTagC2v = new PacketTagC2v();
  std::vector<uint32_t> reqsIds;
  for (uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
    {
      reqsIds.push_back(data);
    }
  pktTagC2v->SetReqsIds(reqsIds);
  dataTxApp->SetPacketTag(pktTagC2v);
  m_appContainer.Add(dataTxApp);
  m_remoteHost->AddApplication(dataTxApp);
  dataTxApp->SetStartTime(Seconds(0));

//  requestQueue.pop_front();
}

void
VanetCsVfcExperiment::CommandSetup (int argc, char **argv)
{
  NS_LOG_FUNCTION_NOARGS ();

  CommandLine cmd;

  // allow command line overrides
//  cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
//  cmd.AddValue ("CSVfileName2", "The name of the CSV output file name2", m_CSVfileName2);
  cmd.AddValue ("simTime", "Simulation end time", m_totalSimTime);
  cmd.AddValue ("nObu", "Number of obu nodes", m_nObuNodes);
  cmd.AddValue ("nRsu", "Number of rsu nodes", m_nRsuNodes);
  cmd.AddValue ("obuTxp", "Transmit power (dB), e.g. txp=7.5", m_obuTxp);
  cmd.AddValue ("obuTxRange", "Transmit Range of OBU", m_obuTxRange);
  cmd.AddValue ("rsuTxRange", "Transmit Range of RSU", m_rsuTxRange);
  cmd.AddValue ("bsTxp", "Transmit power (dB) of BS", m_bsTxp);
  cmd.AddValue ("bsTxRange", "Transmit Range of BS", m_bsTxRange);
//  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("phyMode", "Wifi Phy mode", m_phyMode);
  cmd.AddValue ("cwd", "current workspace directory", m_cwd);
  cmd.AddValue ("outputDir", "output directory", m_outputDir);
  cmd.AddValue ("mobilityFile", "Ns2 movement trace file", m_mobilityFile);
  cmd.AddValue ("enterExitStatsFile", "enter and exit statistics for each vehicle", m_enterExitStatsFile);
  cmd.AddValue ("timeNodeidLocationFile", "location of each vehicle at all time", m_timeNodeidLocationFile);
  cmd.AddValue ("mobTraceFile", "Mobility Trace file", m_mobLogFile);
  cmd.AddValue ("verbose", "0=quiet;1=verbose", m_verbose);
  cmd.AddValue ("isFileOut", "output to file", m_isFileOut);

  cmd.AddValue ("asciiTrace", "Dump ASCII Trace data", m_asciiTrace);
  cmd.AddValue ("pcap", "Create PCAP files for all nodes", m_pcap);

  cmd.AddValue ("time", "time", m_time);
  cmd.AddValue ("space", "space", m_space);
  cmd.AddValue ("mapSize", "size of map", m_mapSize);
  cmd.AddValue ("mapWidth", "width of map", m_mapWidth);
  cmd.AddValue ("mapHeight", "hieght of map", m_mapHeight);

  cmd.AddValue ("schePeriod", "scheduling period", m_schedulingPeriod);
  cmd.AddValue ("schemeName", "scheduling algorithm name", m_schemeName);
  cmd.AddValue ("schemeParamAdjust", "scheduling algorithm name", m_schemeParamAdjust);

  cmd.AddValue ("initReqsScheme", "Scheme for initializing data", m_initReqsScheme);
  cmd.AddValue ("packetSize", "size of packet", m_packetSize);
  cmd.AddValue ("dataSize", "size of data", m_dataSize);
  cmd.AddValue ("globalDbSize", "size of global database", m_globalDbSize);
  cmd.AddValue ("minCacheSize", "minimum value of size of cache", m_minCacheSize);
  cmd.AddValue ("maxCacheSize", "maxmum value of size of cache", m_maxCacheSize);
  cmd.AddValue ("nClique", "number of clique broadcast within every period", m_numClique);

  cmd.Parse (argc, argv);

  NS_ASSERT_MSG((m_schemeName.compare(Scheme_1) == 0 || m_schemeName.compare(Scheme_2) == 0 || m_schemeName.compare(Scheme_3) == 0), "scheme name must be \"cs-vfc\", \"ncb\" or \"ma\"");
}

void
VanetCsVfcExperiment::SetupTraceFile ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_traceMobility)
    {
//      // open trace file for output
//      m_os.open (m_mobLogFile.c_str ());
//
//      // Configure callback for logging
//      Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
//		       MakeBoundCallback (&VanetCsVfcExperiment::CourseChange, &m_os));

      AsciiTraceHelper ascii;
      MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (m_trName));
    }
}

void
VanetCsVfcExperiment::SetupLogging ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_log != 0)
    {
//      // open trace file for output
//      m_os.open (m_mobLogFile.c_str ());

      // Enable logging from the ns2 helper
      LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_DEBUG);

      Packet::EnablePrinting ();
    }
}

void
VanetCsVfcExperiment::ConfigureDefaults ()
{
  NS_LOG_FUNCTION_NOARGS ();

  Config::SetDefault ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue ("2048bps"));
  Config::SetDefault ("ns3::CsmaNetDevice::Mtu",  StringValue ("65000"));

  //Set Non-unicastMode rate to unicast mode
//  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (m_phyMode));
}

void
VanetCsVfcExperiment::SetupDevices ()
{
  NS_LOG_FUNCTION_NOARGS ();

#if Friis_Propagation_Loss_Model
  YansWifiChannelHelper wifiChannelHelper1 = YansWifiChannelHelper::Default ();
  wifiChannelHelper1.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannelHelper1.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5.9e9));
  Ptr<YansWifiChannel> wifiChannel1 = wifiChannelHelper1.Create ();
#elif Range_Propagation_Loss_Model
  // RangePropagationLossModel
//  Config::SetDefault ("ns3::RangePropagationLossModel::MaxRange", DoubleValue (m_rsuTxRange));
  Ptr<YansWifiChannel> wifiChannel1 = CreateObject<YansWifiChannel> ();
  wifiChannel1->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());
  Ptr<RangePropagationLossModel> loss = CreateObject<RangePropagationLossModel> ();
  loss->SetAttribute("MaxRange", DoubleValue (m_rsuTxRange));
  wifiChannel1->SetPropagationLossModel (loss);
#else
  YansWifiChannelHelper wifiChannelHelper1 = YansWifiChannelHelper::Default ();
  Ptr<YansWifiChannel> wifiChannel1 = wifiChannelHelper1.Create ();
#endif // Friis_Propagation_Loss_Model

  YansWifiPhyHelper wifiPhy1 =  YansWifiPhyHelper::Default ();
  wifiPhy1.SetChannel (wifiChannel1);
  // ns-3 supports generate a pcap trace
  wifiPhy1.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  NqosWaveMacHelper wifi80211pMac1 = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p1 = Wifi80211pHelper::Default ();
  if (m_verbose)
    {
      wifi80211p1.EnableLogComponents ();      // Turn on all Wifi 802.11p logging
    }

  // OBU 802.11p
  std::ostringstream phyMode1;
  phyMode1 << "OfdmRate" << PhyRate_Max << "MbpsBW10MHz";
  wifi80211p1.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue (phyMode1.str()),
                                      "ControlMode",StringValue (phyMode1.str()));
  wifiPhy1.Set ("TxPowerStart",DoubleValue (m_obuTxp));
  wifiPhy1.Set ("TxPowerEnd", DoubleValue (m_obuTxp));
  m_obuDevicesTx = wifi80211p1.Install (wifiPhy1, wifi80211pMac1, m_obuNodes);
  m_obuDevicesRx = wifi80211p1.Install (wifiPhy1, wifi80211pMac1, m_obuNodes);

  // RSU 802.11p
  std::ostringstream phyMode2;
  phyMode2 << "OfdmRate" << PhyRate_Max << "MbpsBW10MHz";
  wifi80211p1.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue (phyMode2.str()),
                                      "ControlMode",StringValue (phyMode2.str()));
  wifiPhy1.Set ("TxPowerStart",DoubleValue (m_rsuTxp));
  wifiPhy1.Set ("TxPowerEnd", DoubleValue (m_rsuTxp));
  m_rsu80211pDevicesTx = wifi80211p1.Install (wifiPhy1, wifi80211pMac1, m_rsuNodes);
  m_rsu80211pDevicesRx = wifi80211p1.Install (wifiPhy1, wifi80211pMac1, m_rsuNodes);

//  // RSU CSMA
//  CsmaHelper csmaHelper;
//  csmaHelper.SetChannelAttribute ("DataRate", StringValue ("1000Mbps"));
//  csmaHelper.SetChannelAttribute ("Delay", StringValue ("0.1ms"));
//  m_rsuCsmaDevices = csmaHelper.Install(m_rsuNodes);

  InternetStackHelper internet;
  internet.Install(m_obuNodes);
//  internet.Install(m_rsuNodes);

  Ipv4AddressHelper addressHelper;
//  addressHelper.SetBase ("192.168.0.0", "255.255.0.0");
//  m_rsuCsmaInterfaces = addressHelper.Assign (m_rsuCsmaDevices);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("2000Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));
  m_rsuStarHelper = MyPointToPointStarHelper (m_rsuNodes, pointToPoint);
  m_rsuStarHelper.InstallStack(internet);
  m_rsuStarHelper.AssignIpv4Addresses (Ipv4AddressHelper ("192.168.0.0", "255.255.0.0"));
  Address hubLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), m_i2IPort));
  PacketSinkHelper packetSinkHelper (PROTOCOL_UDP, hubLocalAddress);
  ApplicationContainer hubApp = packetSinkHelper.Install (m_rsuStarHelper.GetHub ());
  hubApp.Start (Seconds (0.01));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      std::ostringstream ossAddr;
      ossAddr << m_rsuStarHelper.GetSpokeIpv4Address(i);
      m_ipaddr2FogIdxCsmaMap[ossAddr.str()] = i;
    }

  // configure mobility
  SetupObuMobilityNodes ();
  SetupRsuMobilityNodes ();

#if Lte_Enable
  // -------------------------------------------LTE----------------------------------------------------
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity",UintegerValue (320));

  m_lteHelper = CreateObject<LteHelper> ();
  m_epcHelper = CreateObject<PointToPointEpcHelper> ();
  m_lteHelper->SetEpcHelper (m_epcHelper);

  m_pgw = m_epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  m_remoteHostNodes.Create (1);
  m_remoteHost = m_remoteHostNodes.Get (0);
  internet.Install (m_remoteHostNodes);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (m_pgw, m_remoteHost);
  addressHelper.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = addressHelper.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  m_remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (m_remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  m_enbNodes.Create(1);
  m_ueNodes.Add(m_obuNodes);

  // configure mobility
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc3 = CreateObject<ListPositionAllocator> ();
  positionAlloc3->Add (Vector (1800.0, 1800.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc3);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (m_remoteHost);

  positionAlloc3 = CreateObject<ListPositionAllocator> ();
  positionAlloc3->Add (Vector (1800.0, 1600.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc3);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (m_pgw);

  positionAlloc3 = CreateObject<ListPositionAllocator> ();
  positionAlloc3->Add (Vector (1600, 1600, 0.0));
  mobility.SetPositionAllocator (positionAlloc3);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (m_enbNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer m_enbLteDevices = m_lteHelper->InstallEnbDevice (m_enbNodes);
  NetDeviceContainer m_ueLteDevices = m_lteHelper->InstallUeDevice (m_ueNodes);

  // Assign Ip to UE
  m_ueInterface = m_epcHelper->AssignUeIpv4Address (m_ueLteDevices);

  for (uint32_t u = 0; u < m_ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = m_ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (m_epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE to eNodeB
  m_lteHelper->Attach (m_ueLteDevices);  // side effect: the default EPS bearer will be activated
#else
  // Create a single RemoteHost
  m_remoteHostNodes.Create (1);
  m_remoteHost = m_remoteHostNodes.Get (0);

  m_ueNodes.Add(m_obuNodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (m_mapWidth / 2, m_mapHeight / 2, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (m_remoteHost);

  // BS
//  YansWifiChannelHelper wifiChannelHelper2 = YansWifiChannelHelper::Default ();
//  wifiChannelHelper2.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
//  wifiChannelHelper2.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));
//  Ptr<YansWifiChannel> wifiChannel2 = wifiChannelHelper2.Create ();

//  Config::SetDefault ("ns3::FriisPropagationLossModel::Frequency", DoubleValue (5e9));
  Ptr<YansWifiChannel> wifiChannel2 = CreateObject<YansWifiChannel> ();
  wifiChannel2->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());
  Ptr<RangePropagationLossModel> lossModel = CreateObject<RangePropagationLossModel> ();
  lossModel->SetAttribute("MaxRange", DoubleValue (m_bsTxRange));
//  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel> ();
  wifiChannel2->SetPropagationLossModel (lossModel);

  YansWifiPhyHelper wifiPhy2 =  YansWifiPhyHelper::Default ();
  wifiPhy2.SetChannel (wifiChannel2);
  // ns-3 supports generate a pcap trace
  wifiPhy2.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  NqosWaveMacHelper wifi80211pMac2 = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p2 = Wifi80211pHelper::Default ();

  std::ostringstream phyMode3;
  phyMode3 << "OfdmRate" << PhyRate_Max << "MbpsBW10MHz";
  wifi80211p1.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue (phyMode3.str()),
                                      "ControlMode",StringValue (phyMode3.str()));

  wifiPhy2.Set ("TxPowerStart",DoubleValue (m_bsTxp));
  wifiPhy2.Set ("TxPowerEnd", DoubleValue (m_bsTxp));
  NetDeviceContainer remoteDev = wifi80211p1.Install (wifiPhy2, wifi80211pMac2, m_remoteHostNodes);

  internet.Install (m_remoteHostNodes);
//  addressHelper.SetBase ("1.0.0.0", "255.0.0.0");
  addressHelper.SetBase ("10.2.0.0", "255.255.0.0");
  Ipv4InterfaceContainer internetIpIfaces = addressHelper.Assign (remoteDev);
  m_remoteHostAddr = internetIpIfaces.GetAddress (0);

//  Ipv4StaticRoutingHelper ipv4RoutingHelper;
//  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (m_remoteHost->GetObject<Ipv4> ());
//  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("10.2.0.0"), Ipv4Mask ("255.255.0.0"), 1);

  // OBU 802.11p NetDevice communicating with BS
  wifiPhy2.Set ("TxPowerStart",DoubleValue (m_bsTxp));
  wifiPhy2.Set ("TxPowerEnd", DoubleValue (m_bsTxp));
  NetDeviceContainer m_ueLteDevices = wifi80211p2.Install (wifiPhy2, wifi80211pMac2, m_ueNodes);
  m_ueInterface = addressHelper.Assign (m_ueLteDevices);

  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      std::ostringstream ossAddr;
      ossAddr << m_ueInterface.GetAddress(i);
      m_ipaddr2VehIdxLteMap[ossAddr.str()] = i;
    }
#endif

  // NOTE:The ip address assignment of DSRC must be placed after "epcHelper->AssignUeIpv4Address", otherwise UE's upload line will be invalid, why??????
  // Assign Ip to OBUs and RSUs
  // we may have a lot of nodes, and want them all
  // in same subnet, to support broadcast
  addressHelper.SetBase ("10.3.0.0", "255.255.0.0");
  m_rsu80211pInterfacesTx = addressHelper.Assign (m_rsu80211pDevicesTx);
  m_rsu80211pInterfacesRx = addressHelper.Assign (m_rsu80211pDevicesRx);

  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      std::ostringstream ossAddr1;
      ossAddr1 << m_rsu80211pInterfacesTx.GetAddress(i);
      m_ipaddr2FogIdxDsrcMap[ossAddr1.str()] = i;

      std::ostringstream ossAddr2;
      ossAddr2 << m_rsu80211pInterfacesRx.GetAddress(i);
      m_ipaddr2FogIdxDsrcMap[ossAddr2.str()] = i;
    }

  m_obuInterfacesTx = addressHelper.Assign (m_obuDevicesTx);
  m_obuInterfacesRx = addressHelper.Assign (m_obuDevicesRx);

  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      std::ostringstream ossAddr1;
      ossAddr1 << m_obuInterfacesTx.GetAddress(i);
      m_ipaddr2VehIdxDsrcMap[ossAddr1.str()] = i;

      std::ostringstream ossAddr2;
      ossAddr2 << m_obuInterfacesRx.GetAddress(i);
      m_ipaddr2VehIdxDsrcMap[ossAddr2.str()] = i;
    }
}

void
VanetCsVfcExperiment::AssignIpAddresses ()
{
  NS_LOG_FUNCTION_NOARGS ();
}
void
VanetCsVfcExperiment::SetupObuMobilityNodes ()
{
  NS_LOG_FUNCTION_NOARGS ();

  // Create Ns2MobilityHelper with the specified trace log file as parameter
  Ns2MobilityHelper ns2 = Ns2MobilityHelper (m_mobilityFile);
  ns2.Install (m_obuNodes.Begin(), m_obuNodes.End()); // configure movements for each OBU node, while reading trace file

//  nodesMoving.resize (m_nObuNodes, 0);

  NodeContainer::Iterator i = m_obuNodes.Begin ();
  for (; i != m_obuNodes.End(); ++i)
    {
      Ptr<Node> node = (*i);
      uint32_t id = node->GetId();
      std::ostringstream oss;
      oss << "/NodeList/" << id << "/$ns3::MobilityModel/CourseChange";
      Config::Connect (oss.str(), MakeCallback (&VanetCsVfcExperiment::CourseChange, this));
    }
}

void
VanetCsVfcExperiment::SetupRsuMobilityNodes ()
{
  NS_LOG_FUNCTION_NOARGS ();

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  // m_nRsuNodes == 9
  int size = 3;
  double baseX = -500.0;
  double baseY = -500.0;
  double delta = 1000.0;

  if (m_space.compare("jiangbei") == 0)
    {
      NS_LOG_INFO("map space:" << "jiangbei");
      baseX = -416.0;
      baseY = -416.0;
      delta = 833.0;
    }

  if (m_nRsuNodes == 16)
    {
      size = 4;
//      baseX = -1000.0;
//      baseY = -1000.0;
//      delta = 1000.0;
    }
  else if (m_nRsuNodes == 25)
    {
      size = 5;
    }

  double x = baseX, y = baseY;
  for (int i = 0; i < size; i++)
    {
      x = x + delta;
      y = baseY;
      for (int j = 0; j < size; j++)
	{
	  y = y + delta;
	  positionAlloc->Add (Vector (x, y, 0.0));
	}
    }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (m_rsuNodes);
}

void
VanetCsVfcExperiment::ReceivePacketWithAddr (std::string context, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr)
{
  m_receiveCount++;

  size_t start = context.find("/", 0);
  start = context.find("/", start + 1);
  size_t end = context.find("/", start + 1);
  std::string nodeIdStr = context.substr(start + 1, end - start - 1);
  uint32_t nodeId = 0;
  sscanf(nodeIdStr.c_str(), "%d", &nodeId);

//  Ptr<Node> node = NodeList::GetNode(nodeId);

  if (m_schemeName.compare(Scheme_1) == 0)
    {
      ReceivePacketOnSchemeFACS (nodeId, packet, srcAddr, destAddr);
    }
  else if (m_schemeName.compare(Scheme_2) == 0)
    {
      ReceivePacketOnSchemeNcb (nodeId, packet, srcAddr, destAddr);
    }
  else if (m_schemeName.compare(Scheme_3) == 0)
    {
      ReceivePacketOnSchemeIsxd (nodeId, packet, srcAddr, destAddr);
    }
}

void
VanetCsVfcExperiment::ReceivePacketOnSchemeFACS (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr)
{
  std::ostringstream oss;
  oss << "At time:" <<Simulator::Now ().GetSeconds ();
#if Print_Log_Header_On_Receive
  if (InetSocketAddress::IsMatchingType (srcAddr))
    {
      InetSocketAddress inetSrcAddr = InetSocketAddress::ConvertFrom (srcAddr);

      oss << " src:";
      Ipv4Address ipv4Addr = inetSrcAddr.GetIpv4 ();
      std::ostringstream ossAddr;
      ossAddr << ipv4Addr;
      if (m_ipaddr2VehIdxDsrcMap.count(ossAddr.str()))
	{
	  oss << " obu " << m_ipaddr2VehIdxDsrcMap[ossAddr.str()];
	}
      else if (m_ipaddr2VehIdxLteMap.count(ossAddr.str()))
	{
	  oss << " obu " << m_ipaddr2VehIdxLteMap[ossAddr.str()];
	}
      else if (m_ipaddr2FogIdxDsrcMap.count(ossAddr.str()))
	{
	  oss << " rsu " << m_ipaddr2FogIdxDsrcMap[ossAddr.str()];
	}
      else if (m_ipaddr2FogIdxCsmaMap.count(ossAddr.str()))
	{
	  oss << " rsu " << m_ipaddr2FogIdxCsmaMap[ossAddr.str()];
	}
      else
	{
	  oss << " " << ipv4Addr;
	}
    }
  if (InetSocketAddress::IsMatchingType (destAddr))
    {
      InetSocketAddress inetDestAddr = InetSocketAddress::ConvertFrom (destAddr);

      oss << " dest:";
      Ipv4Address ipv4Addr = inetDestAddr.GetIpv4 ();
      std::ostringstream ossAddr;
      ossAddr << ipv4Addr;
      if (m_ipaddr2VehIdxDsrcMap.count(ossAddr.str()))
	{
	  oss << " obu " << m_ipaddr2VehIdxDsrcMap[ossAddr.str()];
	}
      else if (m_ipaddr2VehIdxLteMap.count(ossAddr.str()))
	{
	  oss << " obu " << m_ipaddr2VehIdxLteMap[ossAddr.str()];
	}
      else if (m_ipaddr2FogIdxDsrcMap.count(ossAddr.str()))
	{
	  oss << " rsu " << m_ipaddr2FogIdxDsrcMap[ossAddr.str()];
	}
      else if (m_ipaddr2FogIdxCsmaMap.count(ossAddr.str()))
	{
	  oss << " rsu " << m_ipaddr2FogIdxCsmaMap[ossAddr.str()];
	}
      else
	{
	  oss << " " << ipv4Addr;
	}
    }
#endif
  using vanet::PacketHeader;
  PacketHeader recvHeader;
  Ptr<Packet> pktCopy = packet->Copy();
  pktCopy->RemoveHeader(recvHeader);
  uint32_t broadcastId = recvHeader.GetBroadcastId();
  uint32_t dataPktId = recvHeader.GetDataPktId();
  uint32_t dataPktSeq = recvHeader.GetDataPktSeq();
  uint32_t dataSize = recvHeader.GetDataSize();

  oss << " bId: " << broadcastId;
  oss << " bytes: " << packet->GetSize();
  oss << " dataPktId: " << dataPktId;
  oss << " dataPktSeq: " << dataPktSeq;

#if Print_Msg_Type
  switch (recvHeader.GetType())
  {
    case PacketHeader::MessageType::NOT_SET:
      oss << " MessageType::NOT_SET ";
      break;
    case PacketHeader::MessageType::REQUEST:
      oss << " MessageType::REQUEST ";
      break;
    case PacketHeader::MessageType::ACK:
      oss << " MessageType::ACK ";
      break;
    case PacketHeader::MessageType::DATA_C2V:
      oss << " MessageType::DATA_C2V ";
      break;
    case PacketHeader::MessageType::DATA_V2F:
      oss << " MessageType::DATA_V2F ";
      break;
    case PacketHeader::MessageType::DATA_F2F:
      oss << " MessageType::DATA_F2F ";
      break;
    case PacketHeader::MessageType::DATA_F2V:
      oss << " MessageType::DATA_F2V ";
      break;
    case PacketHeader::MessageType::DATA_V2V:
      oss << " MessageType::DATA_V2V ";
      break;
    default:
      NS_ASSERT_MSG(false, "MessageType must be NOT_SET, REQUEST, ACK, DATA_C2V, DATA_V2F, DATA_F2F, DATA_F2V or DATA_V2V");
  }
#endif

  if (recvHeader.GetType() == PacketHeader::MessageType::DATA_C2V)
    {
      NS_ASSERT_MSG(m_vehId2IndexMap.count(nodeId), "nodeId:" << nodeId);
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);

      if (m_vehsToSatisfy[broadcastId].count(obuIdx) == 0) return;

      // Judging whether the vehicle is in the service area or not
      if (!m_vehsEnter[obuIdx])
	{
	  NS_LOG_DEBUG("obu " << obuIdx << " is out of service area!");
	  return;
	}

      m_isReceiveC2vPkt[broadcastId][obuIdx] = true;

      using vanet::PacketTagC2v;
      PacketTagC2v pktTagC2v;
      pktCopy->RemovePacketTag(pktTagC2v);
      std::vector<uint32_t> reqIds = pktTagC2v.GetReqsIds();

      if (!isDataPktDone[broadcastId][nodeId][dataPktId])
	{
	  m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] += packet->GetSize();
	  if (m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] < (dataSize * Data_Recv_Rate)) return;
	  isDataPktDone[broadcastId][nodeId][dataPktId] = true;
	}
      else
	{
	  return;
	}

#if Print_Received_Data
      oss << " data:[";
      for (uint32_t data : reqIds)
	{
	  oss << " " << data;
	}
      oss << "]";
#endif // Print_Received_Data

      if (!m_isEncoded[broadcastId])
	{
	  uint32_t dataIdx = m_dataToBroadcast[broadcastId][0];
	  if (m_vehsReqs[obuIdx].count(dataIdx) != 0)
	    {
	      m_vehsReqs[obuIdx].erase(dataIdx);
	      m_vehsCaches[obuIdx].insert(dataIdx);
	      DoStats(obuIdx, dataIdx);

	      NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx
			    << ", data:" << dataIdx << ", MsgType:C2V" << ", Encoded: " << m_isEncoded[broadcastId]);
	    }
	}
      else
	{
	  NS_ASSERT(m_FacsResultMap.count(broadcastId) != 0 && m_FacsResultMap[broadcastId].count(obuIdx) != 0);

	  uint32_t dataReqedIdx = (uint32_t)m_FacsResultMap[broadcastId][obuIdx].dataReqed;
	  int dataMergedIdx = m_FacsResultMap[broadcastId][obuIdx].dataMerged;
	  int transMode = m_FacsResultMap[broadcastId][obuIdx].transMode;

	  NS_ASSERT_MSG(transMode == -1 || transMode == 1 || transMode == 2, "transmission mode must be -1, 1 or 2.");
	  if (transMode == -1)
	    {
	      if (m_vehsReqs[obuIdx].count(dataReqedIdx) != 0)
		{
		  m_vehsReqs[obuIdx].erase(dataReqedIdx);
		  m_vehsCaches[obuIdx].insert(dataReqedIdx);
		  DoStats(obuIdx, dataReqedIdx);

		  NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx << ", data:" << dataReqedIdx
				<< ", MsgType:C2V" << ", Encoded: " << m_isEncoded[broadcastId]
				<< ", transMode:" << transMode);
		}
	    }
	  else
	    {
	      if (m_vehsCaches[obuIdx].count((uint32_t)dataReqedIdx) != 0)
		{
		  if (m_vehsReqs[obuIdx].count(dataMergedIdx) != 0)
		    {
		      m_vehsReqs[obuIdx].erase(dataMergedIdx);
		      m_vehsCaches[obuIdx].insert(dataMergedIdx);
		      DoStats(obuIdx, dataMergedIdx);

		      NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx << ", dataMergedIdx:" << dataMergedIdx
				    << ", MsgType:C2V" << ", Encoded: " << m_isEncoded[broadcastId]
				    << ", transMode:" << transMode);
		    }
		}
	    }
	}
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_V2V)
    {
      NS_ASSERT_MSG(m_vehId2IndexMap.count(nodeId), "nodeId:" << nodeId);
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);

      using vanet::PacketTagV2v;
      PacketTagV2v pktTagV2v;
      pktCopy->RemovePacketTag(pktTagV2v);

      std::vector<uint32_t> dataIdxs = pktTagV2v.GetDataIdxs();

      if (!isDataPktDone[broadcastId][nodeId][dataPktId])
	{
	  m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] += packet->GetSize();
//	  NS_LOG_INFO ("rcv v2v: " << m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] << " bytes"
//		       << ", proportion: " << (double)m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] / dataSize
//		       << ", packetId: " << dataPktId << ", packetSeq: " << dataPktSeq);
	  if (m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] < (dataSize * Data_Recv_Rate)) return;
	  isDataPktDone[broadcastId][nodeId][dataPktId] = true;
	}
      else
	{
	  return;
	}

      NS_LOG_INFO ("rcv v2v:" << nodeId);

//      uint32_t srcNodeId = pktTagV2v.GetSrcNodeId();
//      Ptr<Node> srcNode = NodeList::GetNode(srcNodeId);
//      Ptr<MobilityModel> srcNodeMob = srcNode->GetObject<MobilityModel> ();
//      Vector pos_srcNode = srcNodeMob->GetPosition ();
//
//      Ptr<Node> destNode = NodeList::GetNode(nodeId);
//      Ptr<MobilityModel> destNodeMob = destNode->GetObject<MobilityModel> ();
//      Vector pos_destNode = destNodeMob->GetPosition ();
//
//      if (CalculateDistance (pos_destNode, pos_srcNode) > m_rsuTxRange )
//      {
//	NS_LOG_DEBUG("distance between OBU(" << vehId2IndexMap[srcNodeId] << ") and OBU(" << obuIdx << "): "
//		     << CalculateDistance (pos_destNode, pos_srcNode));
//	return;
//      }

#if Print_Received_Data
      oss << " data:[";
      for (uint32_t data : dataIdxs)
	{
	  oss << " " << data;
	}
      oss << "]";
#endif // Print_Received_Data

      for (uint32_t dataIdx : dataIdxs)
	{
	  NS_ASSERT(m_FacsResultMap.count(broadcastId) != 0);
	  NS_ASSERT(m_FacsResultMap[broadcastId].count(obuIdx) != 0);
	  uint32_t dataReqedIdx = (uint32_t)m_FacsResultMap[broadcastId][obuIdx].dataReqed;

	  int transMode = m_FacsResultMap[broadcastId][obuIdx].transMode;

	  if (transMode != 1) return;

	  if (dataReqedIdx == dataIdx)
	    {
	      if (m_vehsReqs[obuIdx].count(dataIdx) != 0)
		{
		  m_vehsReqs[obuIdx].erase(dataIdx);
		  m_vehsCaches[obuIdx].insert(dataIdx);
		  DoStats(obuIdx, dataIdx);

		  PacketStats reqStats;
		  if (m_requestStatsMap.count(broadcastId) != 0)
		    {
		      reqStats = m_requestStatsMap.at(broadcastId);
		    }
		  reqStats.IncV2vRxPkts();
		  m_requestStatsMap[broadcastId] = reqStats;

		  m_pktsStats.IncV2vRxPkts();

		  NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx << ", data:" << dataIdx << ", MsgType:V2V");
		}

	      if (m_isReceiveC2vPkt[broadcastId][obuIdx])
		{
		  uint32_t dataMergedIdx = m_FacsResultMap[broadcastId][obuIdx].dataMerged;
		  if (m_vehsReqs[obuIdx].count(dataMergedIdx) != 0)
		    {
		      m_vehsReqs[obuIdx].erase(dataMergedIdx);
		      m_vehsCaches[obuIdx].insert(dataMergedIdx);
		      DoStats(obuIdx, dataMergedIdx);

		      NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx
				    << ", dataMergedIdx:" << dataMergedIdx << ", MsgType:V2V");
		    }
		}
	    }
	}
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_V2F)
    {
//      NS_ASSERT_MSG(vehId2IndexMap.count(nodeId), "fogId:" << fogId);
//      uint32_t fogIdx = fogId2FogIdxMap.at(nodeId);

      using vanet::PacketTagV2f;
      PacketTagV2f pktTagV2f;
      pktCopy->RemovePacketTag(pktTagV2f);

      std::vector<uint32_t> nextObusIdx = pktTagV2f.GetNextObusIdx();
      std::vector<uint32_t> nextRsusIdx = pktTagV2f.GetNextRsusIdx();
      std::vector<uint32_t> dataIdxs = pktTagV2f.GetDataIdxs();

      if (!isDataPktDone[broadcastId][nodeId][dataPktId])
	{
	  m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] += packet->GetSize();
//	  NS_LOG_INFO ("rcv v2f: " << m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] << " bytes"
//		       << ", packetId: " << dataPktId << ", packetSeq: " << dataPktSeq);
	  if (m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] < (dataSize * Data_Recv_Rate)) return;
	  isDataPktDone[broadcastId][nodeId][dataPktId] = true;
	}
      else
	{
	  return;
	}

      NS_LOG_INFO ("rcv v2f");

#if Print_Received_Data
      oss << " data:[";
      for (uint32_t data : dataIdxs)
	{
	  oss << " " << data;
	}
      oss << "]";
#endif // Print_Received_Data
      for (uint32_t nextObuIdx : nextObusIdx)
	{
	  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
	  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
	  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	  InetSocketAddress remote = InetSocketAddress (m_obuInterfacesRx.GetAddress(nextObuIdx), m_i2VPort);
	  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
	  std::ostringstream oss;
	  oss << Rate_V2F << "Mb/s";
	  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
	  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
	  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
	  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_F2V);
	  header.SetDataPktId(m_dataPktId++);
	  header.SetDataSize(m_dataSize);
	  header.SetBroadcastId(m_currentBroadcastId);
	  dataTxApp->SetHeader(header);

	  using vanet::PacketTagF2v;
	  PacketTagF2v *pktTagF2v = new PacketTagF2v();
	  pktTagF2v->SetSrcNodeId(nodeId);
	  pktTagF2v->SetDataIdxs(dataIdxs);
	  dataTxApp->SetPacketTag(pktTagF2v);
	  m_appContainer.Add(dataTxApp);

	  NodeList::GetNode(nodeId)->AddApplication(dataTxApp);
	  dataTxApp->SetStartTime(Seconds(0.0));
	}
      for (uint32_t nextRsuIdx : nextRsusIdx)
	{
	  std::map<uint32_t, std::vector<uint32_t>> destObusIdx = pktTagV2f.GetDestObusIdx();

	  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
	  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
	  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	  InetSocketAddress remote = InetSocketAddress (m_rsuStarHelper.GetSpokeIpv4Address(nextRsuIdx), m_i2IPort);
	  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
	  std::ostringstream oss;
	  oss << Rate_F2F << "Mb/s";
	  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
	  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
	  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
	  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_F2F);
	  header.SetDataPktId(m_dataPktId++);
	  header.SetDataSize(m_dataSize);
	  header.SetBroadcastId(m_currentBroadcastId);
	  dataTxApp->SetHeader(header);

	  using vanet::PacketTagF2f;
	  PacketTagF2f *pktTagF2f = new PacketTagF2f();
	  pktTagF2f->SetDestObusIdx(destObusIdx[nextRsuIdx]);
	  pktTagF2f->SetDataIdxs(dataIdxs);
	  dataTxApp->SetPacketTag(pktTagF2f);
	  m_appContainer.Add(dataTxApp);

	  NodeList::GetNode(nodeId)->AddApplication(dataTxApp);
	  dataTxApp->SetStartTime(Seconds(0.0));

	  NS_LOG_INFO ("send f2f:" << nodeId << " ----> " << m_rsuNodes.Get(nextRsuIdx)->GetId());
	}

//      PacketTagV2f::NextActionType nextAction = pktTagV2f.GetNextActionType();
//
//      if (nextAction == PacketTagV2f::NextActionType::F2V)
//	{
//	  NS_LOG_INFO ("next f2v");
//
//	  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
//	  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
//	  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//	  InetSocketAddress remote = InetSocketAddress (m_obuInterfacesRx.GetAddress(destObuIdx), m_i2VPort);
//	  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
//	  std::ostringstream oss;
//	  oss << Rate_V2F << "Mb/s";
//	  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
//	  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
//	  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
//	  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));
//
//	  using vanet::PacketHeader;
//	  PacketHeader header;
//	  header.SetType(PacketHeader::MessageType::DATA_F2V);
//	  header.SetDataPktId(m_dataPktId++);
//	  header.SetDataSize(m_dataSize);
//	  header.SetBroadcastId(currentBroadcastId);
//	  dataTxApp->SetHeader(header);
//
//	  using vanet::PacketTagF2v;
//	  PacketTagF2v *pktTagF2v = new PacketTagF2v();
//	  pktTagF2v->SetSrcNodeId(nodeId);
//	  pktTagF2v->SetDataIdxs(dataIdxs);
//	  dataTxApp->SetPacketTag(pktTagF2v);
//
//	  NodeList::GetNode(nodeId)->AddApplication(dataTxApp);
//	  dataTxApp->SetStartTime(Seconds(0.0));
//	}
//      else if (nextAction == PacketTagV2f::NextActionType::F2F)
//	{
//	  std::vector<uint32_t> nextRsuIdxs = pktTagV2f.GetNextRsuIdxs();
//	  for (uint32_t rsuIdx : nextRsuIdxs)
//	    {
//	      std::map<uint32_t, uint32_t> destObusIdx = pktTagV2f.GetDestObusIdx();
//	      Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
//	      dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
//	      dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
//	      InetSocketAddress remote = InetSocketAddress (m_rsuStarHelper.GetSpokeIpv4Address(rsuIdx), m_i2IPort);
//	      dataTxApp->SetAttribute ("Remote", AddressValue (remote));
//	      std::ostringstream oss;
//	      oss << Rate_F2F << "Mb/s";
//	      dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
//	      dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
//	      dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
//	      dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));
//
//	      using vanet::PacketHeader;
//	      PacketHeader header;
//	      header.SetType(PacketHeader::MessageType::DATA_F2F);
//	      header.SetDataPktId(m_dataPktId++);
//	      header.SetDataSize(m_dataSize);
//	      header.SetBroadcastId(currentBroadcastId);
//	      dataTxApp->SetHeader(header);
//
//	      using vanet::PacketTagF2f;
//	      PacketTagF2f *pktTagF2f = new PacketTagF2f();
//	      pktTagF2f->SetDestObuIdx(destObusIdx[rsuIdx]);
//	      pktTagF2f->SetDataIdxs(dataIdxs);
//	      dataTxApp->SetPacketTag(pktTagF2f);
//
//	      NodeList::GetNode(nodeId)->AddApplication(dataTxApp);
//	      dataTxApp->SetStartTime(Seconds(0.0));
//
//	      NS_LOG_INFO ("send f2f:" << nodeId << " ----> " << m_rsuNodes.Get(rsuIdx)->GetId());
//	    }
//	}
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_F2F)
    {
      using vanet::PacketTagF2f;
      PacketTagF2f pktTagF2f;
      pktCopy->RemovePacketTag(pktTagF2f);

      std::vector<uint32_t> dataIdxs = pktTagF2f.GetDataIdxs();

      if (!isDataPktDone[broadcastId][nodeId][dataPktId])
	{
	  m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] += packet->GetSize();
//	  NS_LOG_INFO ("rcv f2f: " << m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] << " bytes"
//		       << ", packetId: " << dataPktId << ", packetSeq: " << dataPktSeq);
	  if (m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] < (dataSize * Data_Recv_Rate)) return;
	  isDataPktDone[broadcastId][nodeId][dataPktId] = true;
	}
      else
	{
	  return;
	}

      NS_LOG_INFO ("rcv f2f:" << nodeId);

#if Print_Received_Data
      oss << " data:[";
      for (uint32_t data : dataIdxs)
	{
	  oss << " " << data;
	}
      oss << "]";
#endif // Print_Received_Data

//      Ptr<UdpSender> sender = CreateObject<UdpSender>();
//      sender->SetNode(NodeList::GetNode(nodeId));
//      sender->SetRemote(Ipv4Address ("10.3.255.255"), m_i2VPort);
//      sender->SetDataSize(m_packetSize);
//      sender->Start();
//      using vanet::PacketHeader;
//      PacketHeader header;
//      header.SetType(PacketHeader::MessageType::DATA_F2V);
//      header.SetBroadcastId(broadcastId);
//      sender->SetHeader(header);
//
//      using vanet::PacketTagF2v;
//      PacketTagF2v *pktTagF2v = new PacketTagF2v();
//      pktTagF2v->SetNextActionType(PacketTagF2v::NextActionType::NOT_SET);
//      pktTagF2v->SetSrcNodeId(nodeId);
//      pktTagF2v->SetDataIdxs(dataIdxs);
//      sender->SetPacketTag(pktTagF2v);
//
//      Simulator::ScheduleNow (&UdpSender::Send, sender);

      std::vector<uint32_t> destObusIdx = pktTagF2f.GetDestObusIdx();

      for (uint32_t destObuIdx : destObusIdx)
	{
	  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
	  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
	  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	  InetSocketAddress remote = InetSocketAddress (m_obuInterfacesRx.GetAddress(destObuIdx), m_i2VPort);
	  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
	  std::ostringstream oss;
	  oss << Rate_V2F << "Mb/s";
	  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
	  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
	  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
	  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_F2V);
	  header.SetDataPktId(m_dataPktId++);
	  header.SetDataSize(m_dataSize);
	  header.SetBroadcastId(m_currentBroadcastId);
	  dataTxApp->SetHeader(header);

	  using vanet::PacketTagF2v;
	  PacketTagF2v *pktTagF2v = new PacketTagF2v();
	  pktTagF2v->SetSrcNodeId(nodeId);
	  pktTagF2v->SetDataIdxs(dataIdxs);
	  dataTxApp->SetPacketTag(pktTagF2v);
	  m_appContainer.Add(dataTxApp);

	  NodeList::GetNode(nodeId)->AddApplication(dataTxApp);
	  dataTxApp->SetStartTime(Seconds(0.0));
	}
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_F2V)
    {
      NS_ASSERT_MSG(m_vehId2IndexMap.count(nodeId), "nodeId:" << nodeId);
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);
//      uint32_t fogIdx = fogId2FogIdxMap.at(pktTagF2v.GetFogId());

      if (m_vehsToSatisfy[broadcastId].count(obuIdx) == 0) return;

      using vanet::PacketTagF2v;
      PacketTagF2v pktTagF2v;
      pktCopy->RemovePacketTag(pktTagF2v);

      std::vector<uint32_t> dataIdxs = pktTagF2v.GetDataIdxs();

      if (!isDataPktDone[broadcastId][nodeId][dataPktId])
	{
	  m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] += packet->GetSize();
//	  NS_LOG_INFO ("rcv f2v: " << m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] << " bytes"
//		       << ", packetId: " << dataPktId << ", packetSeq: " << dataPktSeq);
	  if (m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] < (dataSize * Data_Recv_Rate)) return;
	  isDataPktDone[broadcastId][nodeId][dataPktId] = true;
	}
      else
	{
	  return;
	}

      NS_LOG_INFO ("rcv f2v");

#if Print_Received_Data
      oss << " data:[";
      for (uint32_t data : dataIdxs)
	{
	  oss << " " << data;
	}
      oss << "]";
#endif // Print_Received_Data

//      uint32_t srcNodeId = pktTagF2v.GetSrcNodeId();
//      Ptr<Node> srcNode = NodeList::GetNode(srcNodeId);
//      Ptr<MobilityModel> srcNodeMob = srcNode->GetObject<MobilityModel> ();
//      Vector pos_srcNode = srcNodeMob->GetPosition ();
//
//      Ptr<Node> destNode = NodeList::GetNode(nodeId);
//      Ptr<MobilityModel> destNodeMob = destNode->GetObject<MobilityModel> ();
//      Vector pos_destNode = destNodeMob->GetPosition ();
//
//      if (CalculateDistance (pos_destNode, pos_srcNode) > m_rsuTxRange )
//      {
//	NS_LOG_DEBUG("distance between RSU(" << m_vehId2IndexMap[srcNodeId] << ") and OBU(" << obuIdx << "): "
//		     << CalculateDistance (pos_destNode, pos_srcNode));
//	return;
//      }

      for (uint32_t dataIdx : dataIdxs)
	{
	  NS_ASSERT(m_FacsResultMap.count(broadcastId) != 0);
	  NS_ASSERT(m_FacsResultMap[broadcastId].count(obuIdx) != 0);
	  uint32_t dataReqedIdx = (uint32_t)m_FacsResultMap[broadcastId][obuIdx].dataReqed;
	  int transMode = m_FacsResultMap[broadcastId][obuIdx].transMode;

	  if (transMode != 2) return;

	  if (dataReqedIdx == dataIdx)
	    {
	      if (m_vehsReqs[obuIdx].count(dataIdx) != 0)
		{
		  m_vehsReqs[obuIdx].erase(dataIdx);
		  m_vehsCaches[obuIdx].insert(dataIdx);
		  DoStats(obuIdx, dataIdx);

		  NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx << ", data:" << dataIdx << ", MsgType:F2V");

		  PacketStats reqStats;
		  if (m_requestStatsMap.count(broadcastId) != 0)
		    {
		      reqStats = m_requestStatsMap.at(broadcastId);
		    }
		  reqStats.IncV2fRxPkts();
		  m_requestStatsMap[broadcastId] = reqStats;

		  m_pktsStats.IncV2fRxPkts();
		}

	      if (m_isReceiveC2vPkt[broadcastId][obuIdx])
		{
		  uint32_t dataMergedIdx = m_FacsResultMap[broadcastId][obuIdx].dataMerged;
		  if (m_vehsReqs[obuIdx].count(dataMergedIdx) != 0)
		    {
		      m_vehsReqs[obuIdx].erase(dataMergedIdx);
		      m_vehsCaches[obuIdx].insert(dataMergedIdx);
		      DoStats(obuIdx, dataMergedIdx);

		      NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx
				    << ", dataMergedIdx:" << dataMergedIdx << ", MsgType:F2V");
		    }
		}
	    }
	}
    }
  oss << " DbSize:" << m_globalDbSize;
#if Print_Received_Log
  NS_LOG_UNCOND(oss.str());
#endif
}

void
VanetCsVfcExperiment::PrintEdgeType (const EdgeType& edgeType, std::ostringstream& oss)
{
#if Print_Edge_Type
  switch (edgeType)
  {
    case EdgeType::NOT_SET:
      oss << " EdgeType::NOT_SET ";
      break;
    case EdgeType::CONDITION_1:
      oss << " EdgeType::CONDITION_1 ";
      break;
    case EdgeType::CONDITION_2:
      oss << " EdgeType::CONDITION_2 ";
      break;
    case EdgeType::CONDITION_3:
      oss << " EdgeType::CONDITION_3 ";
      break;
    default:
      NS_ASSERT_MSG(false, "EdgeType must be NOT_SET, CONDITION_1, CONDITION_2, or CONDITION_3");
  }
#endif
}

void
VanetCsVfcExperiment::ReceivePacketOnSchemeNcb (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr)
{
  std::ostringstream oss;
  oss << "sim time:" <<Simulator::Now ().GetSeconds () << ", clock: " << (double)(clock()) / CLOCKS_PER_SEC;
#if Print_Log_Header_On_Receive
  if (InetSocketAddress::IsMatchingType (srcAddr))
    {
      InetSocketAddress inetSrcAddr = InetSocketAddress::ConvertFrom (srcAddr);
      oss << " src: " << inetSrcAddr.GetIpv4 ();
    }
  if (InetSocketAddress::IsMatchingType (destAddr))
    {
      InetSocketAddress inetDestAddr = InetSocketAddress::ConvertFrom (destAddr);
      oss << " dest: " << inetDestAddr.GetIpv4 ();
    }
#endif

  using vanet::PacketHeader;
  PacketHeader recvHeader;
  Ptr<Packet> pktCopy = packet->Copy();
  pktCopy->RemoveHeader(recvHeader);
  uint32_t broadcastId = recvHeader.GetBroadcastId();
  uint32_t dataPktId = recvHeader.GetDataPktId();
  uint32_t dataPktSeq = recvHeader.GetDataPktSeq();
  uint32_t dataSize = recvHeader.GetDataSize();

  oss << " bId: " << broadcastId;
  oss << " bytes: " << packet->GetSize();
  oss << " dataPktId: " << dataPktId;
  oss << " dataPktSeq: " << dataPktSeq;

#if Print_Msg_Type
  switch (recvHeader.GetType())
  {
    case PacketHeader::MessageType::DATA_C2V:
      oss << " MessageType::DATA_C2V ";
      break;
    case PacketHeader::MessageType::DATA_V2F:
      oss << " MessageType::DATA_V2F ";
      break;
    case PacketHeader::MessageType::DATA_F2V:
      oss << " MessageType::DATA_F2V ";
      break;
    default:
      NS_ASSERT_MSG(false, "MessageType must be DATA_C2V, DATA_V2F or DATA_F2V");
  }
#endif

  if (recvHeader.GetType() == PacketHeader::MessageType::DATA_C2V)
    {
      NS_ASSERT_MSG(m_vehId2IndexMap.count(nodeId), "nodeId:" << nodeId);
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);

      if (m_vehsToSatisfy[broadcastId].count(obuIdx) == 0) return;

      // Judging whether the vehicle is in the service area or not
      if (!m_vehsEnter[obuIdx])
	{
	  NS_LOG_DEBUG("obu " << obuIdx << " is out of service area!");
	  return;
	}

      m_isReceiveC2vPkt[broadcastId][obuIdx] = true;

      using vanet::PacketTagC2v;
      PacketTagC2v pktTagC2v;
      pktCopy->RemovePacketTag(pktTagC2v);
      std::vector<uint32_t> reqIds = pktTagC2v.GetReqsIds();

      if (!isDataPktDone[broadcastId][nodeId][dataPktId])
	{
	  m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] += packet->GetSize();
	  if (m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] < (dataSize * Data_Recv_Rate)) return;
	  isDataPktDone[broadcastId][nodeId][dataPktId] = true;
	}
      else
	{
	  return;
	}

#if Print_Received_Data
      oss << " data:[";
      for (uint32_t data : reqIds)
	{
	  oss << " " << data;
	}
      oss << "]";
#endif // Print_Received_Data

      if (!m_isEncoded[broadcastId])
	{
	  uint32_t dataIdx = m_dataToBroadcast[broadcastId][0];
	  if (m_vehsReqs[obuIdx].count(dataIdx) != 0)
	    {
	      m_vehsReqs[obuIdx].erase(dataIdx);
	      m_vehsCaches[obuIdx].insert(dataIdx);
	      DoStats(obuIdx, dataIdx);

	      NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx
			    << ", data:" << dataIdx << ", MsgType:C2V" << ", Encoded: " << m_isEncoded[broadcastId]);
	    }
	}
      else
	{
	  NS_ASSERT(m_NcbResultMap.count(broadcastId) != 0 && m_NcbResultMap[broadcastId].count(obuIdx) != 0);

	  uint32_t dataReqedIdx1 = (uint32_t)m_NcbResultMap[broadcastId][obuIdx].dataReqed1;
	  int dataReqedIdx2 = m_NcbResultMap[broadcastId][obuIdx].dataReqed2;

	  if (dataReqedIdx2 == -1)
	    {
	      if (m_vehsReqs[obuIdx].count(dataReqedIdx1) != 0)
		{
		  m_vehsReqs[obuIdx].erase(dataReqedIdx1);
		  m_vehsCaches[obuIdx].insert(dataReqedIdx1);
		  DoStats(obuIdx, dataReqedIdx1);

		  NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx << ", data:" << dataReqedIdx1
				<< ", MsgType:C2V" << ", Encoded: " << m_isEncoded[broadcastId]);
		}
	    }
	  else
	    {
	      if (m_vehsCaches[obuIdx].count((uint32_t)dataReqedIdx2) != 0)
		{
		  if (m_vehsReqs[obuIdx].count(dataReqedIdx1) != 0)
		    {
		      m_vehsReqs[obuIdx].erase(dataReqedIdx1);
		      m_vehsCaches[obuIdx].insert(dataReqedIdx1);
		      DoStats(obuIdx, dataReqedIdx1);

		      NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx << ", data:" << dataReqedIdx1
				    << ", MsgType:C2V" << ", Encoded: " << m_isEncoded[broadcastId]);
		    }
		}
	    }
	}
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_V2F)
    {
      using vanet::PacketTagV2f;
      PacketTagV2f pktTagV2f;
      pktCopy->RemovePacketTag(pktTagV2f);

      std::vector<uint32_t> nextObusIdx = pktTagV2f.GetNextObusIdx();
      std::vector<uint32_t> nextRsusIdx = pktTagV2f.GetNextRsusIdx();
      std::vector<uint32_t> dataIdxs = pktTagV2f.GetDataIdxs();

      if (!isDataPktDone[broadcastId][nodeId][dataPktId])
	{
	  m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] += packet->GetSize();
//	  NS_LOG_INFO ("rcv v2f: " << m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] << " bytes"
//		       << ", packetId: " << dataPktId << ", packetSeq: " << dataPktSeq);
	  if (m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] < (dataSize * Data_Recv_Rate)) return;
	  isDataPktDone[broadcastId][nodeId][dataPktId] = true;
	}
      else
	{
	  return;
	}

      NS_LOG_INFO ("rcv v2f");

#if Print_Received_Data
      oss << " data:[";
      for (uint32_t data : dataIdxs)
	{
	  oss << " " << data;
	}
      oss << "]";
#endif // Print_Received_Data
      for (uint32_t nextObuIdx : nextObusIdx)
	{
	  Ptr<DataTxApplication> dataTxApp = CreateObject<DataTxApplication>();
	  dataTxApp->SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1000]"));
	  dataTxApp->SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
	  InetSocketAddress remote = InetSocketAddress (m_obuInterfacesRx.GetAddress(nextObuIdx), m_i2VPort);
	  dataTxApp->SetAttribute ("Remote", AddressValue (remote));
	  std::ostringstream oss;
	  oss << Rate_V2F << "Mb/s";
	  dataTxApp->SetAttribute ("Protocol", TypeIdValue (TypeId::LookupByName (PROTOCOL_UDP)));
	  dataTxApp->SetAttribute ("DataRate", DataRateValue (DataRate (oss.str())));
	  dataTxApp->SetAttribute ("PacketSize", UintegerValue (m_packetSize));
	  dataTxApp->SetAttribute ("MaxBytes", UintegerValue (m_dataSize));

	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_F2V);
	  header.SetDataPktId(m_dataPktId++);
	  header.SetDataSize(m_dataSize);
	  header.SetBroadcastId(m_currentBroadcastId);
	  dataTxApp->SetHeader(header);

	  using vanet::PacketTagF2v;
	  PacketTagF2v *pktTagF2v = new PacketTagF2v();
	  pktTagF2v->SetSrcNodeId(nodeId);
	  pktTagF2v->SetDataIdxs(dataIdxs);
	  dataTxApp->SetPacketTag(pktTagF2v);
	  m_appContainer.Add(dataTxApp);

	  NodeList::GetNode(nodeId)->AddApplication(dataTxApp);
	  dataTxApp->SetStartTime(Seconds(0.0));
	}
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_F2V)
    {
      NS_ASSERT_MSG(m_vehId2IndexMap.count(nodeId), "nodeId:" << nodeId);
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);
//      uint32_t fogIdx = fogId2FogIdxMap.at(pktTagF2v.GetFogId());

      if (m_vehsToSatisfy[broadcastId].count(obuIdx) == 0) return;

      using vanet::PacketTagF2v;
      PacketTagF2v pktTagF2v;
      pktCopy->RemovePacketTag(pktTagF2v);

      std::vector<uint32_t> dataIdxs = pktTagF2v.GetDataIdxs();

      if (!isDataPktDone[broadcastId][nodeId][dataPktId])
	{
	  m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] += packet->GetSize();
//	  NS_LOG_INFO ("rcv f2v: " << m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] << " bytes"
//		       << ", packetId: " << dataPktId << ", packetSeq: " << dataPktSeq);
	  if (m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] < (dataSize * Data_Recv_Rate)) return;
	  isDataPktDone[broadcastId][nodeId][dataPktId] = true;
	}
      else
	{
	  return;
	}

      NS_LOG_INFO ("rcv f2v");

#if Print_Received_Data
      oss << " data:[";
      for (uint32_t data : dataIdxs)
	{
	  oss << " " << data;
	}
      oss << "]";
#endif // Print_Received_Data

      for (uint32_t dataIdx : dataIdxs)
	{
	  NS_ASSERT(m_NcbResultMap.count(broadcastId) != 0);
	  NS_ASSERT(m_NcbResultMap[broadcastId].count(obuIdx) != 0);
	  uint32_t dataReqed2 = (uint32_t)m_NcbResultMap[broadcastId][obuIdx].dataReqed2;

	  if (dataReqed2 == dataIdx)
	    {
	      m_vehsCaches[obuIdx].insert(dataIdx);
	      if (m_vehsReqs[obuIdx].count(dataIdx) != 0)
		{
		  m_vehsReqs[obuIdx].erase(dataIdx);
		  DoStats(obuIdx, dataIdx);

		  NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx << ", dataReqed2:" << dataIdx << ", MsgType:F2V");

		  PacketStats reqStats;
		  if (m_requestStatsMap.count(broadcastId) != 0)
		    {
		      reqStats = m_requestStatsMap.at(broadcastId);
		    }
		  reqStats.IncV2fRxPkts();
		  m_requestStatsMap[broadcastId] = reqStats;

		  m_pktsStats.IncV2fRxPkts();
		}

	      if (m_isReceiveC2vPkt[broadcastId][obuIdx])
		{
		  uint32_t dataReqed1 = m_NcbResultMap[broadcastId][obuIdx].dataReqed1;
		  if (m_vehsReqs[obuIdx].count(dataReqed1) != 0)
		    {
		      m_vehsReqs[obuIdx].erase(dataReqed1);
		      m_vehsCaches[obuIdx].insert(dataReqed1);
		      DoStats(obuIdx, dataReqed1);

		      NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx
				    << ", dataReqed1:" << dataReqed1 << ", MsgType:F2V");
		    }
		}
	    }
	}
    }

  oss << " DbSize:" << m_globalDbSize;
#if Print_Received_Log
  NS_LOG_UNCOND(oss.str());
#endif
}

void
VanetCsVfcExperiment::ReceivePacketOnSchemeIsxd (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr)
{
  using vanet::PacketHeader;
  PacketHeader recvHeader;
  Ptr<Packet> pktCopy = packet->Copy();
  pktCopy->RemoveHeader(recvHeader);
  uint32_t broadcastId = recvHeader.GetBroadcastId();
  uint32_t dataPktId = recvHeader.GetDataPktId();
//  uint32_t dataPktSeq = recvHeader.GetDataPktSeq();
  uint32_t dataSize = recvHeader.GetDataSize();

  if (recvHeader.GetType() == PacketHeader::MessageType::DATA_C2V)
    {
      NS_ASSERT_MSG(m_vehId2IndexMap.count(nodeId), "nodeId:" << nodeId);
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);

      if (m_vehsToSatisfy[broadcastId].count(obuIdx) == 0) return;

      // Judging whether the vehicle is in the service area or not
      if (!m_vehsEnter[obuIdx])
	{
	  NS_LOG_DEBUG("obu " << obuIdx << " is out of service area!");
	  return;
	}

      m_isReceiveC2vPkt[broadcastId][obuIdx] = true;

      using vanet::PacketTagC2v;
      PacketTagC2v pktTagC2v;
      pktCopy->RemovePacketTag(pktTagC2v);
      std::vector<uint32_t> reqIds = pktTagC2v.GetReqsIds();

      if (!isDataPktDone[broadcastId][nodeId][dataPktId])
	{
	  m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] += packet->GetSize();
	  if (m_accumulativeRcvBytes[broadcastId][nodeId][dataPktId] < (dataSize * Data_Recv_Rate)) return;
	  isDataPktDone[broadcastId][nodeId][dataPktId] = true;
	}
      else
	{
	  return;
	}

      NS_ASSERT(m_IsxdResultMap.count(broadcastId) != 0 && m_IsxdResultMap[broadcastId].count(obuIdx) != 0);

      uint32_t dataReqed = (uint32_t)m_IsxdResultMap[broadcastId][obuIdx].dataReqed;

      if (m_vehsReqs[obuIdx].count(dataReqed) != 0)
	{
	  m_vehsReqs[obuIdx].erase(dataReqed);
	  m_vehsCaches[obuIdx].insert(dataReqed);
	  DoStats(obuIdx, dataReqed);

	  NS_LOG_DEBUG ("✔️" << " bID:" << broadcastId << ", obu:" << obuIdx << ", data:" << dataReqed
			<< ", MsgType:C2V");
	}
    }
}

void
VanetCsVfcExperiment::OnOffTrace (std::string context, Ptr<const Packet> packet)
{
  uint32_t pktBytes = packet->GetSize ();
  m_routingStats.IncTxBytes (pktBytes);
}

// Zipf by xiaoke
bool Zipf(vector <uint32_t> &Encode_Packet, uint32_t D_Num,float Theta, int Encode_Num)
{
  uint32_t i;
  int tmp_num=0;
  float sum_pro=0;
  vector<float> Prob_D;

  for (i=0; i<D_Num;i++)
  {
    Prob_D.push_back(i+1);
    Prob_D[i]=pow((1/Prob_D[i]),Theta);
    sum_pro=sum_pro+Prob_D[i];
  }
  for (i=0; i<Prob_D.size();i++)
  {
    Prob_D[i]=Prob_D[i]/sum_pro;
  }
  for (i=1; i<Prob_D.size(); i++)
  {
    Prob_D[i]=Prob_D[i]+Prob_D[i-1];
  }

  while (tmp_num<Encode_Num)
  {
    uint32_t j=0;
    float k;
    k=rand()%100/(float)101;
    for(j=0;j<D_Num;j++)
    {
      if(j==0 && k<Prob_D[j] && Encode_Packet[j]==0)
	{
	  Encode_Packet[j]=1;
	  tmp_num=tmp_num+1;
	}
    if (j>=1 && Prob_D[j-1]<k && k<Prob_D[j] && Encode_Packet[j]==0)
      {
	Encode_Packet[j]=1;
	tmp_num=tmp_num+1;
      }
    }
  }
  return true;
}

bool zipf_request(vector<vector<uint32_t> >&Request, uint32_t Num[], uint32_t D_Num, uint32_t V_Num, float Theta, uint32_t Encode_Num)
{
  uint32_t i;
  vector<uint32_t> number;
  for (i=0; i<V_Num; i++)
  {
    int temp=0;
    temp=(rand()%(Num[1]-Num[0]))+Num[0];
    number.push_back(temp);

  }

  for(i=0; i<number.size(); i++)
  {
    uint32_t j=0;
    while ( j<=number[i])
    {
      vector<uint32_t> tmp;
      vector <uint32_t> Encode_Packet(D_Num);
      Zipf(Encode_Packet,D_Num,Theta, Encode_Num);
      for (uint32_t k=0; k<D_Num; k++)
	{
	  if ( Encode_Packet[k]==1)
	    {
	      vector<uint32_t>::iterator it;
	      it=find(Request[i].begin(),Request[i].end(),k);
	      if (it==Request[i].end())
		{
		  Request[i].push_back(k);
		  j=Request[i].size();
		}
	    }
	}
    }
    sort(Request[i].begin(),Request[i].end());
  }
  return true;
}

void
VanetCsVfcExperiment::Initialization ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_nRsuNodes == 16)
    {
#if Friis_Propagation_Loss_Model
      m_obuTxp = 137; //128:400m, 132:350m, 137:450m(5.9GHz)
      m_rsuTxp = 137;
#else
      m_obuTxp = 42.1; //36.1:450m
      m_rsuTxp = 42.1;
#endif //Friis_Propagation_Loss_Model
//      m_rsuTxRange = 450;
//      m_obuTxRange = 450;
    }
  else if (m_nRsuNodes == 9)
    {
#if Friis_Propagation_Loss_Model
      m_obuTxp = 137; //128:400m, 132:350m, 137:450m(5.9GHz)
      m_rsuTxp = 137;
#else
      m_obuTxp = 36.1; //36.1:450m
      m_rsuTxp = 36.1;
#endif //Friis_Propagation_Loss_Model
//      m_rsuTxRange = 450;
//      m_obuTxRange = 450;
    }

  m_mobLogFile = m_cwd + "/mobility.log";
  m_trName = m_cwd + "/vanet-cs-vfc2.mob";
  m_animFile = m_cwd + "/vanet-cs-vfc2-animation.xml";

  m_requestStatsEachVeh.resize(m_nObuNodes);
  m_wifiPhyStats = CreateObject<WifiPhyStats> ();

  m_globalDB.resize(m_globalDbSize);
  m_vehsEnter.resize(m_nObuNodes, false);
  m_nodesMoving.resize(m_nObuNodes, false);

  m_vehsEnterExitTime.resize(m_nObuNodes, std::vector<double>(2, 0.0));
  m_nodeidxTimeLocation.resize(m_nObuNodes, std::vector<Vector>((uint32_t)m_totalSimTime + 1));
  m_vehsInitialReqs.resize(m_nObuNodes);
  m_vehsInitialCaches.resize(m_nObuNodes);
  m_vehsReqs.resize(m_nObuNodes);
  m_vehsReqsStatus.resize(m_nObuNodes);
  m_vehsCaches.resize(m_nObuNodes);
  m_vehNeighbors.resize(m_nObuNodes, std::vector<uint8_t>(m_nObuNodes, 0));
  m_c2vRemainingTime.resize(m_nObuNodes, 0.0);
  m_v2vRemainingTime.resize(m_nObuNodes, std::vector<double>(m_nObuNodes, 0.0));
  m_v2fRemainingTime.resize(m_nRsuNodes, std::vector<double>(m_nObuNodes, 0.0));

  m_vehsReqsInCloud.resize(m_nObuNodes);
  m_vehsReqsStasInCloud.resize(m_nObuNodes);
  m_vehsCachesInCloud.resize(m_nObuNodes);
  m_vehsMobInfoInCloud.resize(m_nObuNodes);

  m_fogCluster.resize(m_nRsuNodes);
  m_fogsCaches.resize(m_nRsuNodes);
  m_fogsReqs.resize(m_nRsuNodes);

  m_isFirstSubmit.resize(m_nObuNodes, true);

  m_extraDataForDecoding.resize(m_nObuNodes);

  m_statsEveryPeriod.resize((uint32_t)m_totalSimTime + 10, std::vector<uint32_t>(3, 0));

  // read enter-exit stats file
  std::ifstream infile;
  infile.open(m_enterExitStatsFile, ios::in);
  uint32_t nodeIdx = 0;
  double enterTime = 0.0;
  double exitTime = 0.0;
  while(!infile.eof())
    {
      infile >> nodeIdx >> enterTime >> exitTime;
      if (nodeIdx >= m_nObuNodes) break;
      m_vehsEnterExitTime[nodeIdx][0] = enterTime;
      m_vehsEnterExitTime[nodeIdx][1] = exitTime;
    }
  infile.close();

  // read node location file
  infile.open(m_timeNodeidLocationFile, ios::in);
  uint32_t time = 0;
  while(!infile.eof())
    {
      Vector position;
      infile >> time >> nodeIdx >> position.x >> position.y;
      if (nodeIdx >= m_nObuNodes) break;
      m_nodeidxTimeLocation[nodeIdx][time] = position;
    }
  infile.close();

//  for (uint32_t i = 0; i < m_nObuNodes; i++)
//    {
//      uint32_t timeSize = (uint32_t)m_totalSimTime;
//      for (uint32_t j = 0; j <= timeSize; j++)
//	{
//	  Vector item = m_nodeidxTimeLocation[i][j];
//	  cout << i << " " << j << ":" << item.x << " " << item.y << endl;
//	}
//    }

  // Initializing global data base
  for (uint32_t i = 0; i < m_globalDbSize; i++)
    {
      m_globalDB[i] = i;
    }

  // Initiaizing cache and request set of vehicle
  if (m_initReqsScheme.compare("zipf") == 0)
    {
      uint32_t req_num[2] = {Zipf_Min_Req, Zipf_Max_Req};  //number of request
      float theta = Zipf_Theta;
      uint32_t encode_num=1;  // single data item
      vector<vector<uint32_t> > zipfReqMatrix(m_nObuNodes);  // request matrix
      zipf_request(zipfReqMatrix, req_num, m_globalDbSize, m_nObuNodes, theta, encode_num);
      for (uint32_t i = 0; i < m_nObuNodes; i++)
	{
	  for (uint32_t data : zipfReqMatrix[i])
	    {
	      m_vehsInitialReqs[i].insert(data);
	    }
	}

//      for (uint32_t i = 0; i < m_nObuNodes; i++)
//	{
//	  cout << i << ":";
//	  for (uint32_t data : m_vehsInitialReqs[i])
//	    {
//	      cout << data << " ";
//	    }
//	  cout << endl;
//	}
    }
  else if (m_initReqsScheme.compare("random") == 0)
    {
      Ptr<UniformRandomVariable> urv = CreateObject<UniformRandomVariable>();
      for (uint32_t i = 0; i < m_nObuNodes; i++)
	{
	  urv->SetStream(i);
    //      uint32_t m_minCacheSize = m_globalDbSize / 5;
    //      uint32_t m_maxCacheSize = m_minCacheSize * 1.5;
	  uint32_t localSize = (uint32_t)(urv->GetValue(m_minCacheSize, m_maxCacheSize));
	  std::set<uint32_t> vehReq(m_globalDB.begin(), m_globalDB.end());
	  std::set<uint32_t> vehCache;
	  for (uint32_t j = 1; j <= localSize; j++)
	    {
	      uint32_t cacheIdx = (uint32_t)(urv->GetValue(0, m_globalDbSize));
	      uint32_t data = m_globalDB[cacheIdx];
	      while (vehCache.count(data))
		{
		  cacheIdx = (uint32_t)(urv->GetValue(0, m_globalDbSize));
		  data = m_globalDB[cacheIdx];
		}
	      vehCache.insert(data);
	      vehReq.erase(data);
	    }
	  m_vehsInitialCaches[i] = vehCache;
	  m_vehsInitialReqs[i] = vehReq;
	}
    }
  else if (m_initReqsScheme.compare("test") == 0)
    {
      std::set<uint32_t> vehReq;
      std::set<uint32_t> vehCache(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(0);
      vehCache.erase(0);
      vehReq.insert(2);
      vehCache.erase(2);
      m_vehsInitialCaches[0] = vehCache;
      m_vehsInitialReqs[0] = vehReq;

      vehReq.clear();
      vehCache.clear();
      vehCache.insert(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(1);
      vehCache.erase(1);
      vehReq.insert(2);
      vehCache.erase(2);
      m_vehsInitialCaches[1] = vehCache;
      m_vehsInitialReqs[1] = vehReq;

      vehReq.clear();
      vehCache.clear();
      vehCache.insert(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(2);
      vehCache.erase(2);
      m_vehsInitialCaches[2] = vehCache;
      m_vehsInitialReqs[2] = vehReq;
    }

#if Print_Vehicle_Initial_Request
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      cout << "veh:" << i << ", init reqs:";
      for(uint32_t req : m_vehsInitialReqs[i])
	{
	  cout << " " << req;
	}
      cout << endl;
    }
#endif

#if Print_Vehicle_Initial_Cache
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      cout << "veh:" << i << ", init caches:";
      for(uint32_t cache : m_vehsInitialCaches[i])
	{
	  cout << " " << cache;
	}
      cout << endl;
    }
#endif

#if 1
  // initialize libMAInitialize
  if (m_schemeName.compare(Scheme_1) == 0)
    {
      if(!libFACSInitialize())
      {
	std::cout << "Could not initialize libFACS!" << std::endl;
	return;
      }
    }
  else if (m_schemeName.compare(Scheme_2) == 0)
    {
      if(!libNCBInitialize())
      {
	std::cout << "Could not initialize libNCB!" << std::endl;
	return;
      }
    }
  else if (m_schemeName.compare(Scheme_3) == 0)
    {
      if(!libISXDInitialize())
      {
	std::cout << "Could not initialize libISXD!" << std::endl;
	return;
      }
    }
#endif
}

int
main (int argc, char *argv[])
{
  VanetCsVfcExperiment experiment;

  experiment.Simulate (argc, argv);
  return 0;
}
