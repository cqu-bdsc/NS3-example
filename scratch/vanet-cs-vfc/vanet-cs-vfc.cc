/*
 * vanet-cs-vfc.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: Yang yanning <yang.ksn@gmail.com>
 */

#include "vanet-cs-vfc.h"

using namespace ns3;

VanetCsVfcExperiment::VanetCsVfcExperiment ():
    m_protocol (0),
    m_dlPort (1000),
    m_ulPort (2000),
    m_v2IPort (3000),
    m_i2IPort (4000),
    m_i2VPort (5000),
    m_protocolName ("NONE"),
    m_schemeName (Scheme_1),  // cs-vfc, ncb, genetic
    m_nObuNodes (120),
    m_obuNodes (),
    m_obuTxp (138.1), // 137:450m, 138.1:600m
    m_nRsuNodes (16),
    m_rsuNodes (),
    m_rsuTxp (138.1), // 450m
    m_rsuTxRange (450), // 450m
    m_ueNodes (),
    m_enbNodes (),
    m_remoteHostNodes (),
    m_bsTxp (166.1), // 2122m
    m_phyMode ("OfdmRate6MbpsBW10MHz"),
    m_traceMobility (false),
    m_cwd ("../workspace/vanet-cs-vfc"),
    m_mobilityFile (m_cwd + "/mobility" + "/mobility.tcl"),
    m_mobLogFile (m_cwd + "/mobility.log"),
    m_trName (m_cwd + "/vanet-cs-vfc.mob"),
    m_animFile (m_cwd + "/vanet-cs-vfc-animation.xml"),
    m_plotFileName (m_cwd + "/result/vanet-cs-vfc-plot"),
    m_log (1),
    m_totalSimTime (832.0),
    m_rate ("2048bps"),
    m_wavePacketSize (200),
    m_nonSafetySize (1500),
    m_waveInterval (0.1),
    m_verbose (0),
    m_routingTables (0),
    m_asciiTrace (0),
    m_pcap (0),
    m_timeSpent (0.0),
    m_globalDbSize (100),
    m_minCacheSize (15),
    m_maxCacheSize (25),
    m_numClique (1),
    m_currentBroadcastId (0),
    m_receiveCount (0)
{
}

VanetCsVfcExperiment::~VanetCsVfcExperiment ()
{
  if (m_schemeName.compare(Scheme_3) == 0)
    {
      libMATerminate();

      libMADecodeTerminate();

      mclTerminateApplication();
    }
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
  // todo
}

void
VanetCsVfcExperiment::ConfigureDevices ()
{
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
  // todo
}

void
VanetCsVfcExperiment::ConfigureMobility ()
{
  SetupObuMobilityNodes ();
  SetupRsuMobilityNodes ();
}
void
VanetCsVfcExperiment::ConfigureApplications ()
{
  // config trace to capture app-data (bytes) for
  // routing data, subtracted and used for
  // routing overhead
  std::ostringstream oss;
  oss.str ("");
  oss << "/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx";
  Config::Connect (oss.str (), MakeCallback (&VanetCsVfcExperiment::OnOffTrace, this));

  ApplicationContainer serverApps;
  for (uint32_t i = 0; i < m_nObuNodes; ++i)
    {
#if Lte_Enable
      PacketSinkHelper obuLteDlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (m_ueInterface.GetAddress(i), m_dlPort));
      serverApps.Add (obuLteDlPacketSinkHelper.Install (m_ueNodes.Get(i)));
#else
//      PacketSinkHelper obuDsrcDlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (m_ueInterface.GetAddress(i), m_dlPort));
      PacketSinkHelper obuDsrcDlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress ("10.2.0.0", m_dlPort));
      serverApps.Add (obuDsrcDlPacketSinkHelper.Install (m_ueNodes.Get(i)));
#endif
//      PacketSinkHelper obuDsrcPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (m_obuInterfaces.GetAddress(i), m_i2VPort));
      PacketSinkHelper obuDsrcPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress ("10.3.0.0", m_i2VPort));
      serverApps.Add (obuDsrcPacketSinkHelper.Install (m_obuNodes.Get(i)));
    }
  for (uint32_t i = 0; i < m_nRsuNodes; ++i)
    {
      PacketSinkHelper rsuDsrcPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (m_rsu80211pInterfaces.GetAddress(i), m_v2IPort));
      serverApps.Add (rsuDsrcPacketSinkHelper.Install (m_rsuNodes.Get(i)));
      PacketSinkHelper rsuCsmaPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (m_rsuCsmaInterfaces.GetAddress(i), m_i2IPort));
      serverApps.Add (rsuCsmaPacketSinkHelper.Install (m_rsuNodes.Get(i)));
    }
#if Cloud_Enable
  PacketSinkHelper remoteHostPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (m_remoteHostAddr, m_ulPort));
  serverApps.Add (remoteHostPacketSinkHelper.Install (m_remoteHost));
#endif
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/RxWithAddresses", MakeCallback (&VanetCsVfcExperiment::ReceivePacketWithAddr, this));
  serverApps.Start (Seconds (0.01));
}

void
VanetCsVfcExperiment::ConfigureTracing ()
{
//  WriteCsvHeader ();//todo
  SetupTraceFile ();
  SetupLogging ();
}

void
VanetCsVfcExperiment::ConfigureAnim ()
{
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
VanetCsVfcExperiment::CheckSimulationStatus ()
{
//  if (m_schemeName.compare(Scheme_1) == 0)
//    {
//      bool flag = true;
//      for (std::set<uint32_t> reqs : vehsReqs)
//	{
//	  if (!reqs.empty())
//	    {
//	      flag = false;
//	      break;
//	    }
//	}
//      if (flag)
//	{
//	  Simulator::Stop();
//	}
//    }
//  else if (m_schemeName.compare(Scheme_2) == 0)
//    {
//      std::list<ReqQueueItem>::iterator iter = requestQueue.begin();
//      for (; iter != requestQueue.end(); iter++)
//        {
//	  if (!requestsToMarkGloabal[iter->name]) break;
//        }
//      if (iter == requestQueue.end())
//	{
//	  Simulator::Stop();
//	}
//    }
//  else if (m_schemeName.compare(Scheme_2) == 0)
//    {
//
//    }
}

void
VanetCsVfcExperiment::Run ()
{

  Simulator::Stop (Seconds (m_totalSimTime));

#if Gen_Animation
  ConfigureAnim ();
#endif

  Simulator::Schedule(Seconds(1.0), &VanetCsVfcExperiment::Loop, this);

  Simulator::Run ();
  Simulator::Destroy ();
}

void
VanetCsVfcExperiment::ProcessOutputs ()
{
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

//  std::list<ReqQueueItem>::iterator iter = requestQueue.begin();
//  cout << "----------------------------------------------------------------------------------------" << endl;
//  cout << "requestQueue:";
//  for (; iter != requestQueue.end(); iter++)
//    {
//      cout << " " << iter->name << "=" << requestsToMarkGloabal[iter->name];
//    }
//  cout << endl;
//  cout << "----------------------------------------------------------------------------------------" << endl;
//
//  uint32_t count = 0;
//  std::map<std::string, bool>::iterator iter1 = requestsToMarkGloabal.begin();
//  cout << "requestsToMarkGloabal:";
//  for (; iter1 != requestsToMarkGloabal.end(); iter1++)
//    {
//      if (!iter1->second)
//	{
//	  count++;
//	}
//    }
//  cout << endl;
//  cout << "----------------------------------------------------------------------------------------" << endl;

//    cout << "----------------------------------------------------------------------------------------" << endl;
//    std::map<uint32_t, std::set<std::string>>::iterator iter2 = requestsToMarkWithBid.begin();
//    for (; iter2 != requestsToMarkWithBid.end(); iter2++)
//      {
//	cout << "requestsToMarkWithBid" << "[" << iter2->first << "]:";
//	for (std::string req : iter2->second)
//	  {
//	    cout << " " << req;
//	  }
//	cout << endl;
//      }
//
//    cout << "----------------------------------------------------------------------------------------" << endl;
//    std::map<uint32_t, std::set<uint32_t>>::iterator iter3 = vehsToSatisfy.begin();
//    for (; iter3 != vehsToSatisfy.end(); iter3++)
//      {
//	cout << "vehsToSatisfy" << "[" << iter3->first << "]:";
//	for (uint32_t veh : iter3->second)
//	  {
//	    cout << " " << veh;
//	  }
//	cout << endl;
//      }
//    cout << "----------------------------------------------------------------------------------------" << endl;
//
//  cout << "requestQueue Size:" << requestQueue.size() << endl;

#if Output_Result
#if !Console_Output_Result
  std::ostringstream ossOutputDir;
  ossOutputDir << m_cwd << "/result-" << m_schemeParamAdjust;
  if (m_schemeName.compare(Scheme_3) == 0)
    {
      ossOutputDir << "-" << m_schemeName;
      if (m_schemeParamAdjust.compare(Scheme_Param_Adjust_1) == 0)
	{
	  ossOutputDir << "-gDbSize[" << m_globalDbSize << "]";
	}
      else if (m_schemeParamAdjust.compare(Scheme_Param_Adjust_2) == 0)
	{
	  ossOutputDir << "-minCacheSize[" << m_minCacheSize << "]";
	}
    }
  ossOutputDir << "-clique[" << m_numClique << "]";
  std::string outputDir = ossOutputDir.str();
  if (access(outputDir.c_str(), 0) == -1)
    {
      MkPath(outputDir);
    }
  std::ostringstream oss;
  oss << outputDir << "/" << m_schemeName << ".txt";
  std::ofstream ofs; ///< output stream
  ofs.open (oss.str(), ios::app);
  streambuf* coutbackup;
  coutbackup = std::cout.rdbuf(ofs.rdbuf());
#endif

  std::cout << "globalDbSize: \t " << m_globalDbSize << std::endl;
  std::cout << "TimeSpent: \t\t " << m_timeSpent << std::endl;
  std::cout << "receive_count: \t " << m_receiveCount << std::endl;
  std::cout << "SubmittedReqs: \t " << m_pktsStats.GetSubmittedReqs() << std::endl;
  std::cout << "SatisfiedReqs: \t " << m_pktsStats.GetSatisfiedReqs() << std::endl;
  std::cout << "BroadcastPkts: \t " << m_pktsStats.GetBroadcastPkts() << std::endl;
  std::cout << "CumulativeDelay: " << m_pktsStats.GetCumulativeDelay() << std::endl;

  NS_ASSERT_MSG(m_pktsStats.GetSatisfiedReqs() != 0, "number of satisfied request is 0");
  double ASD = (m_pktsStats.GetCumulativeDelay() / 1e+9) / m_pktsStats.GetSatisfiedReqs();
  std::cout << "ASD: \t" << ASD << "s" << std::endl;

  NS_ASSERT_MSG(m_pktsStats.GetSubmittedReqs() != 0, "number of submitted request is 0");
  double SR = (double)m_pktsStats.GetSatisfiedReqs() / m_pktsStats.GetSubmittedReqs();
  std::cout << "SR: \t" << SR << std::endl;

  NS_ASSERT_MSG(m_pktsStats.GetBroadcastPkts() != 0, "number of broadcast packets is 0");
  double BE = (double)m_pktsStats.GetSatisfiedReqs() / m_pktsStats.GetBroadcastPkts();
  std::cout << "BE: \t" << BE << std::endl;
  std::cout << std::endl;

#if !Console_Output_Result
  ofs.close ();

  std::ostringstream oss1;
  oss1 << outputDir << "/" << m_schemeName << ".dat";
  ofs.open (oss1.str(), ios::app);
//  m_ofs << "DbSize " << "ASD " << "SR " << "BE " << std::endl;
  std::cout << m_globalDbSize << " " << m_minCacheSize << " " << m_maxCacheSize << " " << ASD << " " << SR << " " << BE << std::endl;
  ofs.close ();

  std::cout.rdbuf(coutbackup);
#endif
#endif //Output_Result

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
#if Upload_Enable
  UploadAllVehiclesInfo ();
#endif

  UpdateAllVehsStatus ();
  SubmitAllVehsRequests ();
  UpdateAllFogCluster ();

#if Lte_Enable
  CheckSimulationStatus ();
#endif

#if Loop_Scheduling
  Simulator::Schedule(Seconds(m_schedulingPeriod), &VanetCsVfcExperiment::Loop, this);
#endif
}

void
VanetCsVfcExperiment::SubmitVehRequests(Ptr<Node> obu)
{
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

      m_pktsStats.IncSubmittedReqs(m_vehsReqs[obuIdx].size());

      m_isFirstSubmit[obuIdx] = false;
    }
}

void
VanetCsVfcExperiment::SubmitAllVehsRequests()
{
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
}

void
VanetCsVfcExperiment::UpdateAllFogCluster()
{
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
  std::cout << Simulator::Now() << endl;
  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      total += m_fogCluster[i].size();
    }
  cout << "total: " << total << endl;
#endif

#if Print_Fog_Cluster
  PrintFogCluster();

#if 0
  m_currentBroadcastId++;
  for (uint32_t fogIndex = 0; fogIndex < m_nRsuNodes; fogIndex++)
    {
      if (fogIndex != 3) continue;
      uint32_t index = 0;
      for (uint32_t obuIdx : m_fogCluster[fogIndex])
	{
//	  if (obuIdx != 9) continue;
	  Ptr<UdpSender> sender = CreateObject<UdpSender>();
	  sender->SetNode(m_obuNodes.Get(obuIdx));
	  sender->SetRemote(m_rsu80211pInterfaces.GetAddress(fogIndex), m_v2IPort);
	  sender->SetDataSize(Packet_Size);
	  sender->Start();
	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_V2F);
	  header.SetBroadcastId(m_currentBroadcastId);
	  sender->SetHeader(header);

	  using vanet::PacketTagV2f;
	  PacketTagV2f *pktTagV2f = new PacketTagV2f();
	  pktTagV2f->SetCurrentEdgeType(EdgeType::NOT_SET);
	  pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::NOT_SET);
	  std::vector<uint32_t> dataIdxs;
	  dataIdxs.push_back(111);
	  pktTagV2f->SetDataIdxs(dataIdxs);
	  sender->SetPacketTag(pktTagV2f);

	  Simulator::Schedule (Seconds(index * 0.01), &UdpSender::Send, sender);

	  index++;
	}
    }
#endif
#endif //Print_Fog_Cluster

#if Scheduling
  if (m_schemeName.compare(Scheme_1) == 0)
    {
      ConstructGraphAndBroadcast();
    }
  else if (m_schemeName.compare(Scheme_2) == 0)
    {
//      requestsToMarkGloabal.clear();
//      reqQueHead.clear();
//      dataToBroadcast.clear();
//      requestsToMarkWithBid.clear();
//      vehsToSatisfy.clear();
      for (uint32_t i = 0; i < m_numClique; i++)
	{
	  Simulator::Schedule(Seconds(i * (double)1/m_numClique), &VanetCsVfcExperiment::NcbFunc1, this);
//	  ConstructMostRewardingPktToBroadcast();
	}
    }
  else if (m_schemeName.compare(Scheme_3) == 0)
    {
      MAandBroadcast ();
    }
#endif
}

void
VanetCsVfcExperiment::UpdateAllFogData()
{
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
#endif

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
#endif
	}
    }
}

void
VanetCsVfcExperiment::PrintFogCluster()
{
  if (m_log != 0)
  {
    std::cout << Simulator::Now() << endl;
    for (uint32_t fogIdx = 0; fogIdx < m_nRsuNodes; fogIdx++)
      {
	Ptr<Node> rsu = m_rsuNodes.Get(fogIdx);
	Ptr<MobilityModel> rsuMobility = rsu->GetObject<MobilityModel> ();
	Vector pos_rsu = rsuMobility->GetPosition ();

	std::cout << "fogCluster[" << fogIdx << "]:";
#if 1
	for (uint32_t obuIdx : m_fogCluster[fogIdx])
	  {
	    Ptr<Node> obu = m_obuNodes.Get(obuIdx);
	    Ptr<MobilityModel> obuMobility = obu->GetObject<MobilityModel> ();
	    Vector pos_obu = obuMobility->GetPosition ();

	    std::cout << " " << obuIdx << "(" << CalculateDistance (pos_obu, pos_rsu) << ")";
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
VanetCsVfcExperiment::ConstructGraphAndBroadcast ()
{
#if Construct_Graph_And_Find_Clique_Time_stas
  clock_t startTime, endTime;
  startTime = clock();
#endif
  // update fog request set cache set
  vector<VertexNode> vertices;
  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      for (uint32_t req : m_fogsReqs[i])
	{
	  VertexNode vn(i, req);
	  vertices.push_back(vn);
	}
    }

  if (vertices.empty()) return;

  m_graph = GraphMatrix<VertexNode> (vertices, vertices.size());

  // add edge using condition 1, 2, 3
  VertexNode vn1;
  VertexNode vn2;
  EdgeNode<> en = {.type = EdgeType::NOT_SET, .weight = 1};
  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      vn1.fogIndex = i;
      for (uint32_t req1 : m_fogsReqs[i])
	{
	  vn1.reqDataIndex = req1;
	  vn1.genName();
	  for (uint32_t j = i + 1; j < m_nRsuNodes; j++)
	    {
	      vn2.fogIndex = j;
	      for (uint32_t req2 : m_fogsReqs[j])
		{
		  vn2.reqDataIndex = req2;
		  vn2.genName();

		  if (req2 == req1) // condition 1
		    {
		      en.type = EdgeType::CONDITION_1;
		      m_graph.addEdge(vn1, vn2, en);
		    }
		  else if ((m_fogsCaches[j].count(req1) && m_fogsCaches[i].count(req2))) // condition 2
		    {
		      en.type = EdgeType::CONDITION_2;
		      m_graph.addEdge(vn1, vn2, en);
		    }
		  else if ((m_fogsReqs[j].count(req1) && m_fogsCaches[i].count(req2))
		      || (m_fogsCaches[j].count(req1) && m_fogsReqs[i].count(req2))) // condition 3
		    {
		      en.type = EdgeType::CONDITION_3;
		      m_graph.addEdge(vn1, vn2, en);
		    }
		}
	    }
	}
    }
#if Print_Edge
  m_graph.printEdge();
#endif

#if Search_Clique
//  std::vector<std::vector<VertexNode>> cliques = graphM.getCliquesWithBA(numClique);
  m_cliques.clear();
  m_cliques = m_graph.getCliques(m_numClique);
#if Print_Cliques
  std::cout << "sim time:" << Simulator::Now().GetSeconds() << ", ";
  m_graph.printCliques(m_cliques);
  std::cout << "DbSize:" << m_globalDbSize << std::endl;
#endif
#endif

#if Construct_Graph_And_Find_Clique_Time_stas
  endTime = clock();
  cout << "sim time: " << Now().GetSeconds() << ", clock: " << (double)(endTime - startTime) / CLOCKS_PER_SEC << endl;
#endif

  // broadcast clique to vehicles
  m_broadcastId2cliqueMap.clear();
  m_isDecoding.clear();
  for (std::vector<VertexNode> clique : m_cliques)
    {
      m_currentBroadcastId++;
      m_broadcastId2cliqueMap[m_currentBroadcastId] = clique;
      m_isDecoding[m_currentBroadcastId] = false;
    }

  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      m_extraDataForDecoding[i].clear();
    }

  m_fogIdx2FogReqInCliqueMaps.clear();
  std::map<uint32_t, std::vector<VertexNode>>::iterator iter = m_broadcastId2cliqueMap.begin();
  for (uint32_t index = 0; iter != m_broadcastId2cliqueMap.end(); iter++, index++)
    {
      std::vector<VertexNode> clique = iter->second;

      m_pktsStats.IncBroadcastPkts();

      std::map<uint32_t, uint32_t> fogIdx2FogReqInCliqueMap;
      std::set<uint32_t> broadcastData;
      for (VertexNode vertex : clique)
	{
	  broadcastData.insert(vertex.reqDataIndex);
	  fogIdx2FogReqInCliqueMap.insert(make_pair(vertex.fogIndex, vertex.reqDataIndex));

	  for (uint32_t obuIdx : m_fogCluster[vertex.fogIndex])
	    {
	      std::set<uint32_t> datasNeeded;
	      datasNeeded.insert(broadcastData.begin(), broadcastData.end());
	      if (m_vehsReqs[obuIdx].count(vertex.reqDataIndex))
		{
		  datasNeeded.erase(vertex.reqDataIndex);
		}
	      for (uint32_t cache : m_vehsCaches[obuIdx])
		{
		  datasNeeded.erase(cache);
		}
	      m_extraDataForDecoding[obuIdx].insert(make_pair(iter->first, datasNeeded));
	    }
	}
      m_fogIdx2FogReqInCliqueMaps.insert(make_pair(iter->first, fogIdx2FogReqInCliqueMap));

      if (broadcastData.size() == 0) continue;

#if Lte_Enable

//      using vanet::PacketHeader;
//      PacketHeader header;
//      header.SetType(PacketHeader::MessageType::DATA_C2V);
//      header.SetBroadcastId(iter->first);
//
//      Ptr<UdpSender> sender = CreateObject<UdpSender>();
//      sender->SetNode(m_remoteHost);
//      sender->SetDataSize(Packet_Size);
//      sender->SetHeader(header);
//      for (uint32_t i = 0; i < m_nObuNodes; i++)
//	{
//	  using vanet::PacketTagC2v;
//	  PacketTagC2v *pktTag = new PacketTagC2v();
//	  std::vector<uint32_t> reqsIds;
//	  reqsIds.assign(broadcastData.begin(), broadcastData.end());
//	  pktTag->SetReqsIds(reqsIds);
//
//	  sender->SetRemote(m_ueInterface.GetAddress(i), m_dlPort);
//	  sender->SetPacketTag(pktTag);
//	  sender->Start();
//
//	  Simulator::ScheduleNow (&UdpSender::Send, sender);
//	}

      for (uint32_t i = 0; i < m_nObuNodes; i++)
	{
	  Ptr<UdpSender> sender = CreateObject<UdpSender>();
	  sender->SetNode(m_remoteHost);
	  sender->SetRemote(m_ueInterface.GetAddress(i) , m_dlPort);
	  sender->SetDataSize(Packet_Size);
	  sender->Start();
	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_C2V);
	  header.SetBroadcastId(iter->first);
	  sender->SetHeader(header);

	  using vanet::PacketTagC2v;
	  PacketTagC2v *pktTag = new PacketTagC2v();
	  std::vector<uint32_t> reqsIds;
	  reqsIds.assign(broadcastData.begin(), broadcastData.end());
	  pktTag->SetReqsIds(reqsIds);
	  sender->SetPacketTag(pktTag);

	  Simulator::ScheduleNow (&UdpSender::Send, sender);
	}
#else
      Ptr<UdpSender> sender = CreateObject<UdpSender>();
      sender->SetNode(m_remoteHost);
      sender->SetRemote(Ipv4Address ("10.2.255.255"), m_dlPort);
      sender->SetDataSize(Packet_Size);
      sender->Start();
      using vanet::PacketHeader;
      PacketHeader header;
      header.SetType(PacketHeader::MessageType::DATA_C2V);
      header.SetBroadcastId(iter->first);
      sender->SetHeader(header);

      using vanet::PacketTagC2v;
      PacketTagC2v *pktTag = new PacketTagC2v();
      std::vector<uint32_t> reqsIds;
      reqsIds.assign(broadcastData.begin(), broadcastData.end());
      pktTag->SetReqsIds(reqsIds);
      sender->SetPacketTag(pktTag);

//      Simulator::ScheduleNow (&UdpSender::Send, sender);
      Simulator::Schedule(Seconds(index * (double)1/m_numClique), &UdpSender::Send, sender);
#endif
    }
}

void
VanetCsVfcExperiment::Decode (bool isEncoded, uint32_t broadcastId)
{
  if (!isEncoded)
    {
      if (m_broadcastId2cliqueMap.count(broadcastId) == 0) return;
//      for (VertexNode vertex : broadcastId2cliqueMap.at(broadcastId))
//	{
//	  for (uint32_t obuIdx : fogCluster[vertex.fogIndex])
//	    {
//	      if (vehsReqs[obuIdx].count(vertex.reqDataIndex))
//		{
//		  vehsReqs[obuIdx].erase(vertex.reqDataIndex);
//		  vehsCaches[obuIdx].insert(vertex.reqDataIndex);
//
//		  RecordStats(obuIdx, vertex.reqDataIndex);
//
////		  map<uint32_t, std::set<uint32_t>>::iterator iter = datasNeededForDecodingPerClique[obuIdx].begin();
////		  for (; iter != datasNeededForDecodingPerClique[obuIdx].end(); iter++)
////		    {
////		      iter->second.erase(vertex.reqDataIndex);
////		      if (iter->second.size() == 0)
////			{
////			  if (fogIdx2FogReqInCliqueMaps.count(iter->first)
////			      && fogIdx2FogReqInCliqueMaps.at(iter->first).count(vertex.fogIndex))
////			    {
////			      uint32_t fogReqIdx = fogIdx2FogReqInCliqueMaps.at(iter->first).at(vertex.fogIndex);
////			      if (vehsReqs[obuIdx].count(fogReqIdx) != 0)
////				{
////				  vehsReqs[obuIdx].erase(fogReqIdx);
////				  vehsCaches[obuIdx].insert(fogReqIdx);
////				  RecordStats(obuIdx, fogReqIdx);
////				}
////			    }
////			}
////		    }
//
////		  map<uint32_t, std::set<uint32_t>>::iterator iter = datasNeededForDecodingPerClique[obuIdx].find(broadcastId);
////		  if (iter != datasNeededForDecodingPerClique[obuIdx].end())
////		    {
////		      iter->second.erase(vertex.reqDataIndex);
////		      if (iter->second.size() == 0)
////			{
////			  if (fogIdx2FogReqInCliqueMaps.count(iter->first)
////			      && fogIdx2FogReqInCliqueMaps.at(iter->first).count(vertex.fogIndex))
////			    {
////			      uint32_t fogReqIdx = fogIdx2FogReqInCliqueMaps.at(iter->first).at(vertex.fogIndex);
////			      if (vehsReqs[obuIdx].count(fogReqIdx) != 0)
////				{
////				  vehsReqs[obuIdx].erase(fogReqIdx);
////				  vehsCaches[obuIdx].insert(fogReqIdx);
////				  RecordStats(obuIdx, fogReqIdx);
////				}
////			    }
////			}
////		    }
//		}
//	    }
//
//	}

      VertexNode vertex = m_broadcastId2cliqueMap.at(broadcastId)[0];
      for (uint32_t i = 0; i < m_nObuNodes; i++)
	{
	  uint32_t obuIdx = i;
	  if (m_vehsReqs[obuIdx].count(vertex.reqDataIndex))
	    {
	      m_vehsReqs[obuIdx].erase(vertex.reqDataIndex);
	      m_vehsCaches[obuIdx].insert(vertex.reqDataIndex);

	      DoStats(obuIdx, vertex.reqDataIndex);
	    }
	}
    }
#if 1
  else
    {
      std::set<uint32_t> fogIdxsInCliques; // fog nodes index covered by a clique
      if (m_broadcastId2cliqueMap.count(broadcastId) == 0) return;
      std::vector<VertexNode> clique = m_broadcastId2cliqueMap.at(broadcastId);
      for (VertexNode vertex : clique)
	{
	  fogIdxsInCliques.insert(vertex.fogIndex);
	}
      uint32_t size = clique.size();
#if 1
      for (uint32_t fogIdx1 : fogIdxsInCliques)
	{
	  uint32_t fogReq = m_fogIdx2FogReqInCliqueMaps[broadcastId].at(fogIdx1);
	  bool flag = false;
	  for (uint32_t fogIdx2 : fogIdxsInCliques)
	    {
	      if (flag) break;
	      if (fogIdx1 == fogIdx2) continue;
	      if (m_fogsCaches[fogIdx2].count(fogReq))
		{
		  for (uint32_t obuIdx : m_fogCluster[fogIdx2])
		    {
		      if (m_vehsCaches[obuIdx].count(fogReq))
			{
			  Ptr<UdpSender> sender = CreateObject<UdpSender>();
			  sender->SetNode(m_obuNodes.Get(obuIdx));
			  sender->SetRemote(m_rsu80211pInterfaces.GetAddress(fogIdx2), m_v2IPort);
			  sender->SetDataSize(Packet_Size);
			  sender->Start();
			  using vanet::PacketHeader;
			  PacketHeader header;
			  header.SetType(PacketHeader::MessageType::DATA_V2F);
			  header.SetBroadcastId(broadcastId);
			  sender->SetHeader(header);

			  using vanet::PacketTagV2f;
			  PacketTagV2f *pktTagV2f = new PacketTagV2f();
			  pktTagV2f->SetCurrentEdgeType(EdgeType::NOT_SET);
			  pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2F);
			  std::vector<uint32_t> rsuWaitingServedIdxs;
			  rsuWaitingServedIdxs.push_back(fogIdx1);
			  pktTagV2f->SetNextRsuIdxs(rsuWaitingServedIdxs);
			  std::vector<uint32_t> dataIdxs;
			  dataIdxs.push_back(fogReq);
			  pktTagV2f->SetDataIdxs(dataIdxs);
			  sender->SetPacketTag(pktTagV2f);

			  Simulator::ScheduleNow (&UdpSender::Send, sender);

			  flag = true;
			  break;
			}
		    }
		}
	    }
	}
#endif

#if 0
      for (uint32_t i = 0; i < size; i++)
	{
	  VertexNode vertex = clique[i];
	  for (uint32_t obuIdx : m_fogCluster[vertex.fogIndex])
	    {
	      if (m_vehsReqs[obuIdx].count(vertex.reqDataIndex))
		{
		  map<uint32_t, std::set<uint32_t>>::iterator it = m_extraDataForDecoding[obuIdx].find(broadcastId);
		  NS_ASSERT (it != m_extraDataForDecoding[obuIdx].end());
//		  it->second.erase(vertex.reqDataIndex);

		  for (uint32_t data : it->second)
		    {
		      bool flag = false;
		      for (uint32_t fogIdx : fogIdxsInCliques)
			{
			  if (flag) break;
			  if (fogIdx == vertex.fogIndex) continue;
			  if (m_fogsCaches[fogIdx].count(data))
			    {
			      for (uint32_t obuIdx1 : m_fogCluster[fogIdx])
				{
				  if (m_vehsCaches[obuIdx1].count(data))
				    {
				      Ptr<UdpSender> sender = CreateObject<UdpSender>();
				      sender->SetNode(m_obuNodes.Get(obuIdx));
				      sender->SetRemote(m_rsu80211pInterfaces.GetAddress(fogIdx), m_v2IPort);
				      sender->SetDataSize(Packet_Size);
				      sender->Start();
				      using vanet::PacketHeader;
				      PacketHeader header;
				      header.SetType(PacketHeader::MessageType::DATA_V2F);
				      header.SetBroadcastId(broadcastId);
				      sender->SetHeader(header);

				      using vanet::PacketTagV2f;
				      PacketTagV2f *pktTagV2f = new PacketTagV2f();
				      pktTagV2f->SetCurrentEdgeType(EdgeType::NOT_SET);
				      pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2F);
				      std::vector<uint32_t> rsuWaitingServedIdxs;
				      rsuWaitingServedIdxs.push_back(vertex.fogIndex);
				      pktTagV2f->SetNextRsuIdxs(rsuWaitingServedIdxs);
				      std::vector<uint32_t> dataIdxs;
				      dataIdxs.push_back(data);
				      pktTagV2f->SetDataIdxs(dataIdxs);
				      sender->SetPacketTag(pktTagV2f);

				      Simulator::ScheduleNow (&UdpSender::Send, sender);

				      flag = true;
				      break;
				    }
				}
			    }
			}
		    }
		}
	    }
	}
#endif

      for (uint32_t i = 0; i < size; i++)
	{
	  VertexNode vertex1 = clique[i];
	  for (uint32_t j = i + 1; j < size; j++)
	    {
	      VertexNode vertex2 = clique[j];
	      EdgeType edgeType = m_graph.getEdgeType(vertex1, vertex2);
	      switch(edgeType)
	      {
		case EdgeType::CONDITION_1:
		  {
		    bool flag = false;
		    for (uint32_t k = 0; k < m_nRsuNodes; k++)
		      {
			if (flag) break; //avoid sending packet repeatly

			if (k == vertex1.fogIndex
			    || k == vertex2.fogIndex
			    || 0 == fogIdxsInCliques.count(k)) continue;

			if (m_fogsCaches[k].count(vertex1.reqDataIndex))
			  {
			    for (uint32_t obuIdx : m_fogCluster[k])
			      {
				if (m_vehsCaches[obuIdx].count(vertex1.reqDataIndex))
				  {
				    Ptr<UdpSender> sender = CreateObject<UdpSender>();
				    sender->SetNode(m_obuNodes.Get(obuIdx));
				    sender->SetRemote(m_rsu80211pInterfaces.GetAddress(k), m_v2IPort);
				    sender->SetDataSize(Packet_Size);
				    sender->Start();
				    using vanet::PacketHeader;
				    PacketHeader header;
				    header.SetType(PacketHeader::MessageType::DATA_V2F);
				    header.SetBroadcastId(broadcastId);
				    sender->SetHeader(header);

				    using vanet::PacketTagV2f;
				    PacketTagV2f *pktTagV2f = new PacketTagV2f();
				    pktTagV2f->SetCurrentEdgeType(EdgeType::CONDITION_1);
				    pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2F);
				    std::vector<uint32_t> rsuWaitingServedIdxs;
				    rsuWaitingServedIdxs.push_back(vertex1.fogIndex);
				    rsuWaitingServedIdxs.push_back(vertex2.fogIndex);
				    pktTagV2f->SetNextRsuIdxs(rsuWaitingServedIdxs);
				    std::vector<uint32_t> dataIdxs;
				    dataIdxs.push_back(vertex1.reqDataIndex);
				    pktTagV2f->SetDataIdxs(dataIdxs);
				    sender->SetPacketTag(pktTagV2f);

				    Simulator::ScheduleNow (&UdpSender::Send, sender);

				    flag = true;
				    break;
				  }
			      }
			  }
		      }
		    break;
		  }
		case EdgeType::CONDITION_2:
		  {
		    bool flag1 = false;
		    for (uint32_t obuIdx : m_fogCluster[vertex1.fogIndex])
		      {
			if (m_vehsCaches[obuIdx].count(vertex2.reqDataIndex))
			  {
			    Ptr<UdpSender> sender = CreateObject<UdpSender>();
			    sender->SetNode(m_obuNodes.Get(obuIdx));
			    sender->SetRemote(m_rsu80211pInterfaces.GetAddress(vertex1.fogIndex), m_v2IPort);
			    sender->SetDataSize(Packet_Size);
			    sender->Start();
			    using vanet::PacketHeader;
			    PacketHeader header;
			    header.SetType(PacketHeader::MessageType::DATA_V2F);
			    header.SetBroadcastId(broadcastId);
			    sender->SetHeader(header);

			    using vanet::PacketTagV2f;
			    PacketTagV2f *pktTagV2f = new PacketTagV2f();
			    pktTagV2f->SetCurrentEdgeType(EdgeType::CONDITION_2);
			    pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2V);
			    std::vector<uint32_t> dataIdxs;
			    dataIdxs.push_back(vertex2.reqDataIndex);
			    pktTagV2f->SetDataIdxs(dataIdxs);
			    sender->SetPacketTag(pktTagV2f);

			    Simulator::ScheduleNow (&UdpSender::Send, sender);

			    flag1 = true;
			    break;
			  }
		      }
		    NS_ASSERT(flag1);

		    bool flag2 = false;
		    for (uint32_t obuIdx : m_fogCluster[vertex2.fogIndex])
		      {
			if (m_vehsCaches[obuIdx].count(vertex1.reqDataIndex))
			  {
			    Ptr<UdpSender> sender = CreateObject<UdpSender>();
			    sender->SetNode(m_obuNodes.Get(obuIdx));
			    sender->SetRemote(m_rsu80211pInterfaces.GetAddress(vertex2.fogIndex), m_v2IPort);
			    sender->SetDataSize(Packet_Size);
			    sender->Start();
			    using vanet::PacketHeader;
			    PacketHeader header;
			    header.SetType(PacketHeader::MessageType::DATA_V2F);
			    header.SetBroadcastId(broadcastId);
			    sender->SetHeader(header);

			    using vanet::PacketTagV2f;
			    PacketTagV2f *pktTagV2f = new PacketTagV2f();
			    pktTagV2f->SetCurrentEdgeType(EdgeType::CONDITION_2);
			    pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2V);
			    std::vector<uint32_t> dataIdxs;
			    dataIdxs.push_back(vertex1.reqDataIndex);
			    pktTagV2f->SetDataIdxs(dataIdxs);
			    sender->SetPacketTag(pktTagV2f);

			    Simulator::ScheduleNow (&UdpSender::Send, sender);

			    flag2 = true;
			    break;
			  }
		      }
		    NS_ASSERT(flag2);
		    break;
		  }
		case EdgeType::CONDITION_3:
		  {
		    for (uint32_t obuIdx : m_fogCluster[vertex1.fogIndex])
		      {
			if (m_vehsCaches[obuIdx].count(vertex2.reqDataIndex))
			  {
			    Ptr<UdpSender> sender = CreateObject<UdpSender>();
			    sender->SetNode(m_obuNodes.Get(obuIdx));
			    sender->SetRemote(m_rsu80211pInterfaces.GetAddress(vertex1.fogIndex), m_v2IPort);
			    sender->SetDataSize(Packet_Size);
			    sender->Start();
			    using vanet::PacketHeader;
			    PacketHeader header;
			    header.SetType(PacketHeader::MessageType::DATA_V2F);
			    header.SetBroadcastId(broadcastId);
			    sender->SetHeader(header);

			    using vanet::PacketTagV2f;
			    PacketTagV2f *pktTagV2f = new PacketTagV2f();
			    pktTagV2f->SetCurrentEdgeType(EdgeType::CONDITION_3);
			    pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2V);
			    std::vector<uint32_t> rsuWaitingServedIdxs;
			    rsuWaitingServedIdxs.push_back(vertex2.fogIndex);
			    pktTagV2f->SetNextRsuIdxs(rsuWaitingServedIdxs);
			    std::vector<uint32_t> dataIdxs;
			    dataIdxs.push_back(vertex2.reqDataIndex);
			    pktTagV2f->SetDataIdxs(dataIdxs);
			    sender->SetPacketTag(pktTagV2f);

			    Simulator::ScheduleNow (&UdpSender::Send, sender);

			    break;
			  }
		      }

		    for (uint32_t obuIdx : m_fogCluster[vertex2.fogIndex])
		      {
			if (m_vehsCaches[obuIdx].count(vertex1.reqDataIndex))
			  {
			    Ptr<UdpSender> sender = CreateObject<UdpSender>();
			    sender->SetNode(m_obuNodes.Get(obuIdx));
			    sender->SetRemote(m_rsu80211pInterfaces.GetAddress(vertex2.fogIndex), m_v2IPort);
			    sender->SetDataSize(Packet_Size);
			    sender->Start();
			    using vanet::PacketHeader;
			    PacketHeader header;
			    header.SetType(PacketHeader::MessageType::DATA_V2F);
			    header.SetBroadcastId(broadcastId);
			    sender->SetHeader(header);

			    using vanet::PacketTagV2f;
			    PacketTagV2f *pktTagV2f = new PacketTagV2f();
			    pktTagV2f->SetCurrentEdgeType(EdgeType::CONDITION_3);
			    pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2V);
			    std::vector<uint32_t> dataIdxs;
			    dataIdxs.push_back(vertex1.reqDataIndex);
			    pktTagV2f->SetDataIdxs(dataIdxs);
			    sender->SetPacketTag(pktTagV2f);

			    Simulator::ScheduleNow (&UdpSender::Send, sender);

			    break;
			  }
		      }
		    break;
		  }
		default:
		  NS_ASSERT(false);
		  break;
	      }
	    }
	}
    }
#endif


#if 1
  std::set<uint32_t> fogIdxsInCliques; // fog nodes index covered by a clique
  std::vector<VertexNode> clique = m_broadcastId2cliqueMap.at(broadcastId);
  for (VertexNode vertex : clique)
    {
      fogIdxsInCliques.insert(vertex.fogIndex);
    }
  for (uint32_t fogIdx1 : fogIdxsInCliques)
    {
      uint32_t fogReq = m_fogIdx2FogReqInCliqueMaps[broadcastId].at(fogIdx1);
      bool flag_not_set = false;
      for (uint32_t fogIdx2 : fogIdxsInCliques)
	{
	  if (flag_not_set) break;
	  if (fogIdx1 == fogIdx2) continue;
	  if (m_fogsCaches[fogIdx2].count(fogReq))
	    {
	      for (uint32_t obuIdx : m_fogCluster[fogIdx2])
		{
		  if (m_vehsCaches[obuIdx].count(fogReq))
		    {
		      Ptr<UdpSender> sender = CreateObject<UdpSender>();
		      sender->SetNode(m_obuNodes.Get(obuIdx));
		      sender->SetRemote(m_rsu80211pInterfaces.GetAddress(fogIdx2), m_v2IPort);
		      sender->SetDataSize(Packet_Size);
		      sender->Start();
		      using vanet::PacketHeader;
		      PacketHeader header;
		      header.SetType(PacketHeader::MessageType::DATA_V2F);
		      header.SetBroadcastId(broadcastId);
		      sender->SetHeader(header);

		      using vanet::PacketTagV2f;
		      PacketTagV2f *pktTagV2f = new PacketTagV2f();
		      pktTagV2f->SetCurrentEdgeType(EdgeType::NOT_SET);
		      pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2F);
		      std::vector<uint32_t> rsuWaitingServedIdxs;
		      rsuWaitingServedIdxs.push_back(fogIdx1);
		      pktTagV2f->SetNextRsuIdxs(rsuWaitingServedIdxs);
		      std::vector<uint32_t> dataIdxs;
		      dataIdxs.push_back(fogReq);
		      pktTagV2f->SetDataIdxs(dataIdxs);
		      sender->SetPacketTag(pktTagV2f);

		      Simulator::ScheduleNow (&UdpSender::Send, sender);

		      flag_not_set = true;
		      break;
		    }
		}
	    }
	}
    }
#endif
}

void VanetCsVfcExperiment::DoStats (uint32_t obuIdx, uint32_t dataIdx)
{
  std::map<uint32_t, RequestStatus>::iterator it = m_vehsReqsStatus[obuIdx].find(dataIdx);
  NS_ASSERT_MSG(it != m_vehsReqsStatus[obuIdx].end(), "iter == end()");
  if (it->second.completed) return;
  it->second.completed = true;
  it->second.satisfiedTime = Now().GetDouble();

  m_pktsStats.IncSatisfiedReqs();
  m_pktsStats.IncCumulativeDelay(it->second.satisfiedTime - it->second.submitTime);
}

void
VanetCsVfcExperiment::NcbFunc1 ()
{
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

  m_pktsStats.IncBroadcastPkts();

  Ptr<UdpSender> sender = CreateObject<UdpSender>();
  sender->SetNode(m_remoteHost);
  sender->SetRemote(Ipv4Address ("10.2.255.255"), m_dlPort);
  sender->SetDataSize(Packet_Size);
  sender->Start();
  using vanet::PacketHeader;
  PacketHeader header;
  header.SetType(PacketHeader::MessageType::DATA_C2V);
  header.SetBroadcastId(m_currentBroadcastId);
  sender->SetHeader(header);

  using vanet::PacketTagC2v;
  PacketTagC2v *pktTag = new PacketTagC2v();
  std::vector<uint32_t> reqsIds;
  for (uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
    {
      reqsIds.push_back(data);
    }
//  reqsIds.assign(dataToBroadcast[currentBroadcastId].begin(), dataToBroadcast[currentBroadcastId].end());
  pktTag->SetReqsIds(reqsIds);
  sender->SetPacketTag(pktTag);

  Simulator::ScheduleNow (&UdpSender::Send, sender);

//  requestQueue.pop_front();
}

void
VanetCsVfcExperiment::MAandBroadcast ()
{
  std::set<uint32_t> vehs;
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      if (m_vehsEnter[i]) vehs.insert(i);
    }
  if (vehs.size() == 0) return;

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

  mwSize dims[3] = {m_globalDbSize, m_globalDbSize, vehs.size()};
  mwArray mwVehsCachesMatrixTmp(3, dims, mxDOUBLE_CLASS);
  mwArray mwTick(1, 1, mxUINT32_CLASS);
  uint32_t k = 1;
  for (uint32_t veh : vehs)
    {
      for (uint32_t i = 1; i <= m_globalDbSize; i++)
	{
	  for (uint32_t j = 1; j <= m_globalDbSize; j++)
	    {
	      mwVehsCachesMatrixTmp(i, j, k) = (*mwVehsCachesMatrix)(i, j, veh + 1);
	    }
	}
      k += 1;
    }

  mwTick(1, 1) = m_numClique;

  mwArray mwResult1(mxUINT32_CLASS);
  mwArray mwResult2(1, 1, mxUINT32_CLASS);

  cout << "------------------------------------------" << endl;
  cout << "MA start at " << Now().GetSeconds() << "s" << endl;
  cout << "nClique: " << mwTick.Get(1, 1) << endl;

  MA(2, mwResult1, mwResult2, mwVehsCachesMatrixTmp, mwTick);

  uint32_t time = mwResult2.Get(1, 1);
  cout << "nSolution: " << time << endl;
  for (uint32_t i = 1; i <= time; i++)
    {
      m_currentBroadcastId++;
      std::cout << "bID " << m_currentBroadcastId << ":";

      for (uint32_t j = 1; j <= m_globalDbSize; j++)
	{
	  std::cout << " " << mwResult1(i, j);

	  if ((uint32_t)mwResult1(i, j) == 1)
	    {
	      m_dataToBroadcast[m_currentBroadcastId].push_back(j - 1);
	    }
	}
      std::cout << std::endl;

      m_pktsStats.IncBroadcastPkts();

      Ptr<UdpSender> sender = CreateObject<UdpSender>();
      sender->SetNode(m_remoteHost);
      sender->SetRemote(Ipv4Address ("10.2.255.255"), m_dlPort);
      sender->SetDataSize(Packet_Size);
      sender->Start();
      using vanet::PacketHeader;
      PacketHeader header;
      header.SetType(PacketHeader::MessageType::DATA_C2V);
      header.SetBroadcastId(m_currentBroadcastId);
      sender->SetHeader(header);

      using vanet::PacketTagC2v;
      PacketTagC2v *pktTag = new PacketTagC2v();
      std::vector<uint32_t> reqsIds;
      for (uint32_t data : m_dataToBroadcast[m_currentBroadcastId])
	{
	  reqsIds.push_back(data);
	}
      pktTag->SetReqsIds(reqsIds);
      sender->SetPacketTag(pktTag);

      Simulator::ScheduleNow (&UdpSender::Send, sender);
//      Simulator::Schedule (Seconds((i - 1) * ((double)1 / time)) , &UdpSender::Send, sender);
    }
}

void
VanetCsVfcExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd;

  // allow command line overrides
//  cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
//  cmd.AddValue ("CSVfileName2", "The name of the CSV output file name2", m_CSVfileName2);
  cmd.AddValue ("simtime", "Simulation end time", m_totalSimTime);
  cmd.AddValue ("nObu", "Number of obu nodes", m_nObuNodes);
  cmd.AddValue ("nRsu", "Number of rsu nodes", m_nRsuNodes);
  cmd.AddValue ("txp", "Transmit power (dB), e.g. txp=7.5", m_obuTxp);
//  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("phyMode", "Wifi Phy mode", m_phyMode);
  cmd.AddValue ("cwd", "current workspace directory", m_cwd);
  cmd.AddValue ("traceFile", "Ns2 movement trace file", m_mobilityFile);
  cmd.AddValue ("mobTraceFile", "Mobility Trace file", m_mobLogFile);
  cmd.AddValue ("rate", "Rate", m_rate);
  cmd.AddValue ("verbose", "0=quiet;1=verbose", m_verbose);
  cmd.AddValue ("bsm", "(WAVE) BSM size (bytes)", m_wavePacketSize);
  cmd.AddValue ("interval", "(WAVE) BSM interval (s)", m_waveInterval);

  cmd.AddValue ("asciiTrace", "Dump ASCII Trace data", m_asciiTrace);
  cmd.AddValue ("pcap", "Create PCAP files for all nodes", m_pcap);

  cmd.AddValue ("schePeriod", "scheduling period", m_schedulingPeriod);
  cmd.AddValue ("schemeName", "scheduling algorithm name", m_schemeName);
  cmd.AddValue ("schemeParamAdjust", "scheduling algorithm name", m_schemeParamAdjust);
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
  Config::SetDefault ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (m_rate));

  //Set Non-unicastMode rate to unicast mode
//  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (m_phyMode));
}

void
VanetCsVfcExperiment::SetupDevices ()
{
  YansWifiChannelHelper wifiChannel1 = YansWifiChannelHelper::Default ();
  wifiChannel1.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel1.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5.9e9));

  YansWifiPhyHelper wifiPhy1 =  YansWifiPhyHelper::Default ();
  Ptr<YansWifiChannel> channel1 = wifiChannel1.Create ();
  wifiPhy1.SetChannel (channel1);
  // ns-3 supports generate a pcap trace
  wifiPhy1.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  NqosWaveMacHelper wifi80211pMac1 = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p1 = Wifi80211pHelper::Default ();
  if (m_verbose)
    {
      wifi80211p1.EnableLogComponents ();      // Turn on all Wifi 802.11p logging
    }

  // OBU 802.11p
  wifi80211p1.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue ("OfdmRate9MbpsBW10MHz"),
                                      "ControlMode",StringValue ("OfdmRate9MbpsBW10MHz"));
  wifiPhy1.Set ("TxPowerStart",DoubleValue (m_obuTxp));
  wifiPhy1.Set ("TxPowerEnd", DoubleValue (m_obuTxp));
  m_obuDevicesTx = wifi80211p1.Install (wifiPhy1, wifi80211pMac1, m_obuNodes);

  // RSU 802.11p
  wifi80211p1.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue ("OfdmRate6MbpsBW10MHz"),
                                      "ControlMode",StringValue ("OfdmRate6MbpsBW10MHz"));
  wifiPhy1.Set ("TxPowerStart",DoubleValue (m_rsuTxp));
  wifiPhy1.Set ("TxPowerEnd", DoubleValue (m_rsuTxp));
  m_rsu80211pDevicesTx = wifi80211p1.Install (wifiPhy1, wifi80211pMac1, m_rsuNodes);

  // RSU CSMA
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", StringValue ("200Mbps"));
  csmaHelper.SetChannelAttribute ("Delay", StringValue ("2ms"));
  m_rsuCsmaDevices = csmaHelper.Install(m_rsuNodes);

  InternetStackHelper internet;
  internet.Install(m_obuNodes);
  internet.Install(m_rsuNodes);

  Ipv4AddressHelper addressHelper;
//  // Assign Ip to OBUs and RSUs
//  NS_LOG_UNCOND ("Assigning IP addresses");
//  // we may have a lot of nodes, and want them all
//  // in same subnet, to support broadcast
//  addressHelper.SetBase ("10.2.0.0", "255.255.0.0");
//  m_obuInterfaces = addressHelper.Assign (m_obuDevices);
//  m_rsu80211pInterfaces = addressHelper.Assign (m_rsu80211pDevices);

  addressHelper.SetBase ("192.168.0.0", "255.255.0.0");
  m_rsuCsmaInterfaces = addressHelper.Assign (m_rsuCsmaDevices);

  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      std::ostringstream ossAddr;
      ossAddr << m_rsuCsmaInterfaces.GetAddress(i);
      m_ipaddr2FogIdxCsmaMap[ossAddr.str()] = i;
    }

  // Tracing
//  wifiPhy.EnablePcap ("wave-simple-80211p", devices);

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
#if Three_By_Three
  positionAlloc->Add (Vector (2200.0, 2200.0, 0.0));
#else
  positionAlloc->Add (Vector (1250.0, 1250.0, 0.0));
#endif
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (m_remoteHost);

  // BS
  YansWifiChannelHelper wifiChannel2 = YansWifiChannelHelper::Default ();
  wifiChannel2.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel2.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  YansWifiPhyHelper wifiPhy2 =  YansWifiPhyHelper::Default ();
  Ptr<YansWifiChannel> channel2 = wifiChannel2.Create ();
  wifiPhy2.SetChannel (channel2);
  // ns-3 supports generate a pcap trace
  wifiPhy2.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  NqosWaveMacHelper wifi80211pMac2 = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p2 = Wifi80211pHelper::Default ();


  wifi80211p2.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue ("OfdmRate3MbpsBW10MHz"),
                                      "ControlMode",StringValue ("OfdmRate3MbpsBW10MHz"));

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

  // DSRC's ip address assignment must be placed after "epcHelper->AssignUeIpv4Address", otherwise UE's upload line will be invalid, why??????
  // Assign Ip to OBUs and RSUs
  // we may have a lot of nodes, and want them all
  // in same subnet, to support broadcast
  addressHelper.SetBase ("10.3.0.0", "255.255.0.0");
  m_rsu80211pInterfaces = addressHelper.Assign (m_rsu80211pDevicesTx);

  for (uint32_t i = 0; i < m_nRsuNodes; i++)
    {
      std::ostringstream ossAddr;
      ossAddr << m_rsu80211pInterfaces.GetAddress(i);
      m_ipaddr2FogIdxDsrcMap[ossAddr.str()] = i;
    }

  m_obuInterfaces = addressHelper.Assign (m_obuDevicesTx);

  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {
      std::ostringstream ossAddr;
      ossAddr << m_obuInterfaces.GetAddress(i);
      m_ipaddr2VehIdxDsrcMap[ossAddr.str()] = i;
    }
}

void
VanetCsVfcExperiment::AssignIpAddresses ()
{
  // todo
}
void
VanetCsVfcExperiment::SetupObuMobilityNodes ()
{
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
#if Three_By_Three
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  uint32_t size = sqrt(m_nRsuNodes);
  double base = -300.0;
  double x = base, y = base, delta = 1000.0;
  for (uint32_t i = 0; i < size; i++)
    {
      x = x + delta;
      y = base;
      for (uint32_t j = 0; j < size; j++)
	{
	  y = y + delta;
	  positionAlloc->Add (Vector (x, y, 0.0));
	}
    }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (m_rsuNodes);

#else
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

//  int size = 4;
//  double base = -533.0;
//  double delta = 733.0;

  int size = 3;
  double baseX = -900.0;
  double baseY = -840.0;
  double delta = 1100.0;

//  int size = 2;
//  double base = -625.0;
//  double delta = 1250.0;
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
#endif
}

Ptr<Socket>
VanetCsVfcExperiment::SetupPacketSink (Ipv4Address addr, uint16_t port, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&VanetCsVfcExperiment::ReceivePacket, this));

  return sink;
}

void
VanetCsVfcExperiment::SetupMessages ()
{
//  // Setup routing transmissions
//  OnOffHelper onoff ("ns3::UdpSocketFactory",Address ());
//  onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
//  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
//
//  // Setup OBU sink on 80211p devices
//  for (uint32_t i = 0; i < m_nObuNodes; i++)
//    {
//      Ptr<Socket> sink = SetupPacketSink (Ipv4Address::GetAny (), m_port, m_obuNodes.Get (i));
//    }
//
//  // Setup RSU sink on CSMA devices
//  for (uint32_t i = 0; i < m_nRsuNodes; i++)
//    {
//      Ptr<Socket> sink = SetupPacketSink (m_rsuCsmaInterfaces.GetAddress(i), m_port, m_rsuNodes.Get (i));
//    }
//
//  // test RSU broadcast
//  Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
//  for (uint32_t i = 0; i < m_nRsuNodes; i++)
//    {
//      var->SetStream (i);
//      AddressValue remoteAddress (InetSocketAddress ("10.2.255.255", m_port));
//      onoff.SetAttribute ("Remote", remoteAddress);
//
//      ApplicationContainer temp = onoff.Install (m_rsuNodes.Get (i));
//      temp.Start (Seconds (var->GetValue (1.0,2.0)));
//      temp.Stop (Seconds (m_TotalSimTime));
//    }
//
//  // test LTE message
//  uint16_t dlPort = 1234;
//  for (uint32_t i =0; i < m_nRsuNodes; i++)
//    {
//      Ptr<Socket> sink = SetupPacketSink (Ipv4Address::GetAny(), dlPort, m_ueNodes.Get (i));
//    }
//  Ptr<UniformRandomVariable> var1 = CreateObject<UniformRandomVariable> ();
//  for (uint32_t i = 0; i < m_ueInterface.GetN(); i++)
//    {
//      var1->SetStream (i);
//      InetSocketAddress dest = InetSocketAddress (m_ueInterface.GetAddress(i), dlPort);
//      onoff.SetAttribute ("Remote", AddressValue(dest));
//      ApplicationContainer temp = onoff.Install (m_remoteHostNodes.Get(0));
//      temp.Start (Seconds (var1->GetValue (1.0,2.0)));
//      temp.Stop (Seconds (10));
//    }

  std::cout << "sim time:" << Simulator::Now().GetSeconds() << ", event: send data." << std::endl;

  Ptr<UdpSender> sender = CreateObject<UdpSender>();
  sender->SetNode(m_remoteHost);
  sender->SetRemote(Ipv4Address ("10.2.255.255"), m_dlPort);
  sender->SetDataSize(Packet_Size);
  sender->Start();
  using vanet::PacketHeader;
  PacketHeader header;
  header.SetType(PacketHeader::MessageType::DATA_C2V);
  sender->SetHeader(header);

  /**
   * broadcast data size (uint32_t):	4 * byte
   * broadcast data (uint32_t):		4 * byte * broadcastData.size()
   */
  uint32_t dataSize = 4 + 4;
  vanet::ByteBuffer bytes(dataSize);
  bytes.WriteU32(1);
  bytes.WriteU32(static_cast<uint32_t>(Simulator::Now().GetSeconds()));
  sender->SetFill(bytes.GetBufferData(), dataSize);
  Simulator::ScheduleNow (&UdpSender::Send, sender);
}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address srcAddress)
{
  std::ostringstream oss;

  oss << "time=" << Simulator::Now ().GetSeconds () << "s " << "node " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (srcAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (srcAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void
VanetCsVfcExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address srcAddress;
  while ((packet = socket->RecvFrom (srcAddress)))
    {
      uint32_t RxRoutingBytes = packet->GetSize ();
      m_routingStats.IncRxBytes (RxRoutingBytes);
      m_routingStats.IncRxPkts ();
      if (m_log != 0)
        {
          NS_LOG_UNCOND ("protocol=" << m_protocolName + " " + PrintReceivedPacket (socket, packet, srcAddress));
        }
    }
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
      ReceivePacketOnSchemeCsVfc (nodeId, packet, srcAddr, destAddr);
    }
  else if (m_schemeName.compare(Scheme_2) == 0)
    {
      ReceivePacketOnSchemeNcb (nodeId, packet, srcAddr, destAddr);
    }
  else if (m_schemeName.compare(Scheme_3) == 0)
    {
      ReceivePacketOnSchemeMA (nodeId, packet, srcAddr, destAddr);
    }
}

void
VanetCsVfcExperiment::ReceivePacketOnSchemeCsVfc (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr)
{
//  receive_count++;
//
//  size_t start = context.find("/", 0);
//  start = context.find("/", start + 1);
//  size_t end = context.find("/", start + 1);
//  std::string nodeIdStr = context.substr(start + 1, end - start - 1);
//  uint32_t nodeId = 0;
//  sscanf(nodeIdStr.c_str(), "%d", &nodeId);
//
////  Ptr<Node> node = NodeList::GetNode(nodeId);
//
  std::ostringstream oss;
  oss << "sim time:" <<Simulator::Now ().GetSeconds () << ", clock: " << (double)(clock()) / CLOCKS_PER_SEC;
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

  oss << " bId: " << broadcastId;

#if Print_Msg_Type
  switch (recvHeader.GetType())
  {
    case PacketHeader::MessageType::NOT_SET:
      oss << " MessageType::NOT_SET ";
      break;
    case PacketHeader::MessageType::REQUEST:
      oss << " MessageType::REQUEST ";
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
    default:
      NS_ASSERT_MSG(false, "MessageType must be NOT_SET, REQUEST, DATA_C2V, DATA_V2F, DATA_F2F or DATA_F2V");
  }
#endif

  if (recvHeader.GetType() == PacketHeader::MessageType::REQUEST)
    {
      uint32_t pktSize = pktCopy->GetSize();
      uint8_t buffer[pktSize];
      pktCopy->CopyData(buffer, pktSize);
      vanet::ByteBuffer bytes(buffer, pktSize);

      uint32_t obuId = bytes.ReadU32();
      uint32_t obuIdx = m_vehId2IndexMap.at(obuId);
      Vector3D obuPos (bytes.ReadDouble(), bytes.ReadDouble(), 0);
      m_vehsMobInfoInCloud[obuIdx] = obuPos;

      uint32_t reqsSize = bytes.ReadU32();
      for (uint32_t i = 0; i < reqsSize; i++)
	{
	  m_vehsReqsInCloud[obuIdx].insert(bytes.ReadU32());
	}
      uint32_t cachesSize = bytes.ReadU32();
      for (uint32_t i = 0; i < cachesSize; i++)
	{
	  m_vehsCachesInCloud[obuIdx].insert(bytes.ReadU32());
	}

#if Print_Received_Data_Cloud
      oss << " veh_id=" << obuId
	  << " veh_pos_x=" << obuPos.x
	  << " veh_pos_y=" << obuPos.y;
      oss << std::endl;
      oss << "requests:";
      for (uint32_t req : m_vehsReqsInCloud[obuIdx])
	{
	  oss << " " << req;
	}
      oss << std::endl;
      oss << "caches:";
      for (uint32_t cache : m_vehsCachesInCloud[obuIdx])
	{
	  oss << " " << cache;
	}
#endif
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_C2V)
    {
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);

      // Judging whether the vehicle is in the service area or not
      if (!m_vehsEnter[obuIdx]) return;

      using vanet::PacketTagC2v;
      PacketTagC2v pktTagC2v;
      pktCopy->RemovePacketTag(pktTagC2v);
      std::vector<uint32_t> reqIds = pktTagC2v.GetReqsIds();
      uint32_t broadcastDataNum = reqIds.size();

      if (broadcastDataNum == 0) return;

      // Judging whether the broadcasted data is encoded or not
      bool isEncoded = broadcastDataNum == 1 ? false : true;
      oss << " Encoded: " << isEncoded;
      if (!m_isDecoding[broadcastId])
	{
//	  for (uint32_t i = 0; i < m_nObuNodes; i++)
//	    {
//	      std::set<uint32_t> datasNeeded;
//	      datasNeeded.insert(reqIds.begin(), reqIds.end());
//	      uint32_t fogReqIdx = fogIdx2FogReqInCliqueMaps.at(broadcastId).at(vehIdx2FogIdxMap.at(i));
//	      datasNeeded.erase(fogReqIdx);
//	      for (uint32_t cache : vehsCaches[i])
//		{
//		  datasNeeded.erase(cache);
//		}
//	      datasNeededForDecodingPerClique[i].insert(make_pair(broadcastId, datasNeeded));
//	    }

	  m_isDecoding[broadcastId] = true;
	  Decode(isEncoded, broadcastId);
	}
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_V2F)
    {
      using vanet::PacketTagV2f;
      PacketTagV2f pktTagV2f;
      pktCopy->RemovePacketTag(pktTagV2f);

//      std::cout << oss.str ();
//      std::cout << V2F "fogIdx:" << nodeId <<",";
//      pktTag.Print(std::cout);
//      std::cout << endl;

      EdgeType edgeType = pktTagV2f.GetCurrentEdgeType();
      PacketTagV2f::NextActionType nextAction = pktTagV2f.GetNextActionType();

      PrintEdgeType(edgeType, oss);

      if (edgeType == EdgeType::NOT_SET)
	{
	  if (nextAction == PacketTagV2f::NextActionType::F2V)
	    {
	      Ptr<UdpSender> sender = CreateObject<UdpSender>();
	      sender->SetNode(NodeList::GetNode(nodeId));
	      sender->SetRemote(Ipv4Address("10.3.255.255"), m_i2VPort);
	      sender->SetDataSize(Packet_Size);
	      sender->Start();
	      using vanet::PacketHeader;
	      PacketHeader header;
	      header.SetType(PacketHeader::MessageType::DATA_F2V);
	      header.SetBroadcastId(broadcastId);
	      sender->SetHeader(header);

	      using vanet::PacketTagF2v;
	      PacketTagF2v *pktTagF2v = new PacketTagF2v();
	      pktTagF2v->SetCurrentEdgeType(EdgeType::NOT_SET);
	      pktTagF2v->SetNextActionType(PacketTagF2v::NextActionType::NOT_SET);
	      pktTagF2v->SetSrcNodeId(nodeId);
	      pktTagF2v->SetDataIdxs(pktTagV2f.GetDataIdxs());
	      sender->SetPacketTag(pktTagF2v);

	      Simulator::ScheduleNow (&UdpSender::Send, sender);
	    }
	  else if (nextAction == PacketTagV2f::NextActionType::F2F)
	    {
	      std::vector<uint32_t> rsuWaitingServedIdxs = pktTagV2f.GetNextRsusIdx();
	      for (uint32_t rsuIdx : rsuWaitingServedIdxs)
		{
		  Ptr<UdpSender> sender = CreateObject<UdpSender>();
		  sender->SetNode(NodeList::GetNode(nodeId));
		  sender->SetRemote(m_rsuCsmaInterfaces.GetAddress(rsuIdx), m_i2IPort);
		  sender->SetDataSize(Packet_Size);
		  sender->Start();
		  using vanet::PacketHeader;
		  PacketHeader header;
		  header.SetType(PacketHeader::MessageType::DATA_F2F);
		  header.SetBroadcastId(broadcastId);
		  sender->SetHeader(header);

		  using vanet::PacketTagF2f;
		  PacketTagF2f *pktTagF2f = new PacketTagF2f();
		  pktTagF2f->SetCurrentEdgeType(EdgeType::NOT_SET);
		  pktTagF2f->SetDataIdxs(pktTagV2f.GetDataIdxs());
		  sender->SetPacketTag(pktTagF2f);

		  Simulator::ScheduleNow (&UdpSender::Send, sender);
		}
	    }
	}
      else if (edgeType == EdgeType::CONDITION_1)
	{
	  std::vector<uint32_t> rsuWaitingServedIdxs = pktTagV2f.GetNextRsusIdx();
	  for (uint32_t rsuIdx : rsuWaitingServedIdxs)
	    {
	      Ptr<UdpSender> sender = CreateObject<UdpSender>();
	      sender->SetNode(NodeList::GetNode(nodeId));
	      sender->SetRemote(m_rsuCsmaInterfaces.GetAddress(rsuIdx), m_i2IPort);
	      sender->SetDataSize(Packet_Size);
	      sender->Start();
	      using vanet::PacketHeader;
	      PacketHeader header;
	      header.SetType(PacketHeader::MessageType::DATA_F2F);
	      header.SetBroadcastId(broadcastId);
	      sender->SetHeader(header);

	      using vanet::PacketTagF2f;
	      PacketTagF2f *pktTagF2f = new PacketTagF2f();
	      pktTagF2f->SetCurrentEdgeType(EdgeType::CONDITION_1);
	      pktTagF2f->SetDataIdxs(pktTagV2f.GetDataIdxs());
	      sender->SetPacketTag(pktTagF2f);

	      Simulator::ScheduleNow (&UdpSender::Send, sender);
	    }
	}
//      else if (nextAction == PacketTagV2f::NextActionType::F2V)
      else if (edgeType == EdgeType::CONDITION_2)
	{
	  Ptr<UdpSender> sender = CreateObject<UdpSender>();
	  sender->SetNode(NodeList::GetNode(nodeId));
	  sender->SetRemote(Ipv4Address("10.3.255.255"), m_i2VPort);
	  sender->SetDataSize(Packet_Size);
	  sender->Start();
	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_F2V);
	  header.SetBroadcastId(broadcastId);
	  sender->SetHeader(header);

	  using vanet::PacketTagF2v;
	  PacketTagF2v *pktTagF2v = new PacketTagF2v();
	  pktTagF2v->SetCurrentEdgeType(EdgeType::CONDITION_2);
	  pktTagF2v->SetNextActionType(PacketTagF2v::NextActionType::NOT_SET);
	  pktTagF2v->SetSrcNodeId(nodeId);
	  pktTagF2v->SetDataIdxs(pktTagV2f.GetDataIdxs());
	  sender->SetPacketTag(pktTagF2v);

	  Simulator::ScheduleNow (&UdpSender::Send, sender);
	}
      else if (edgeType == EdgeType::CONDITION_3)
	{
	  if (nextAction == PacketTagV2f::NextActionType::F2V)
	    {
	      Ptr<UdpSender> sender = CreateObject<UdpSender>();
	      sender->SetNode(NodeList::GetNode(nodeId));
	      sender->SetRemote(Ipv4Address("10.3.255.255"), m_i2VPort);
	      sender->SetDataSize(Packet_Size);
	      sender->Start();
	      using vanet::PacketHeader;
	      PacketHeader header;
	      header.SetType(PacketHeader::MessageType::DATA_F2V);
	      header.SetBroadcastId(broadcastId);
	      sender->SetHeader(header);

	      using vanet::PacketTagF2v;
	      PacketTagF2v *pktTagF2v = new PacketTagF2v();
	      pktTagF2v->SetCurrentEdgeType(EdgeType::CONDITION_3);
	      pktTagF2v->SetNextActionType(PacketTagF2v::NextActionType::V2F);
	      pktTagF2v->SetSrcNodeId(nodeId);
	      pktTagF2v->SetRsuWaitingServedIdxs(pktTagV2f.GetNextRsusIdx());
	      pktTagF2v->SetDataIdxs(pktTagV2f.GetDataIdxs());
	      sender->SetPacketTag(pktTagF2v);

	      Simulator::ScheduleNow (&UdpSender::Send, sender);
	    }
	  else if (nextAction == PacketTagV2f::NextActionType::F2F)
	    {
	      std::vector<uint32_t> rsuWaitingServedIdxs = pktTagV2f.GetNextRsusIdx();
	      for (uint32_t rsuIdx : rsuWaitingServedIdxs)
		{
		  Ptr<UdpSender> sender = CreateObject<UdpSender>();
		  sender->SetNode(NodeList::GetNode(nodeId));
		  sender->SetRemote(m_rsuCsmaInterfaces.GetAddress(rsuIdx), m_i2IPort);
		  sender->SetDataSize(Packet_Size);
		  sender->Start();
		  using vanet::PacketHeader;
		  PacketHeader header;
		  header.SetType(PacketHeader::MessageType::DATA_F2F);
		  header.SetBroadcastId(broadcastId);
		  sender->SetHeader(header);

		  using vanet::PacketTagF2f;
		  PacketTagF2f *pktTagF2f = new PacketTagF2f();
		  pktTagF2f->SetCurrentEdgeType(EdgeType::CONDITION_3);
		  pktTagF2f->SetDataIdxs(pktTagV2f.GetDataIdxs());
		  sender->SetPacketTag(pktTagF2f);

		  Simulator::ScheduleNow (&UdpSender::Send, sender);
		}
	    }
	}

    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_F2F)
    {
      using vanet::PacketTagF2f;
      PacketTagF2f pktTagF2f;
      pktCopy->RemovePacketTag(pktTagF2f);

//      std::cout << oss.str ();
//      std::cout << " F2F fogIdx:" << nodeId <<",";
//      pktTag.Print(std::cout);
//      std::cout << endl;

      EdgeType edgeType = pktTagF2f.GetCurrentEdgeType();

      PrintEdgeType(edgeType, oss);

      if (edgeType == EdgeType::NOT_SET)
	{
	  Ptr<UdpSender> sender = CreateObject<UdpSender>();
	  sender->SetNode(NodeList::GetNode(nodeId));
	  sender->SetRemote(Ipv4Address ("10.3.255.255"), m_i2VPort);
	  sender->SetDataSize(Packet_Size);
	  sender->Start();
	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_F2V);
	  header.SetBroadcastId(broadcastId);
	  sender->SetHeader(header);

	  using vanet::PacketTagF2v;
	  PacketTagF2v *pktTagF2v = new PacketTagF2v();
	  pktTagF2v->SetCurrentEdgeType(EdgeType::NOT_SET);
	  pktTagF2v->SetNextActionType(PacketTagF2v::NextActionType::NOT_SET);
	  pktTagF2v->SetSrcNodeId(nodeId);
	  pktTagF2v->SetDataIdxs(pktTagF2f.GetDataIdxs());
	  sender->SetPacketTag(pktTagF2v);

	  Simulator::ScheduleNow (&UdpSender::Send, sender);
	}
      else if (edgeType == EdgeType::CONDITION_1)
	{
	  Ptr<UdpSender> sender = CreateObject<UdpSender>();
	  sender->SetNode(NodeList::GetNode(nodeId));
	  sender->SetRemote(Ipv4Address ("10.3.255.255"), m_i2VPort);
	  sender->SetDataSize(Packet_Size);
	  sender->Start();
	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_F2V);
	  header.SetBroadcastId(broadcastId);
	  sender->SetHeader(header);

	  using vanet::PacketTagF2v;
	  PacketTagF2v *pktTagF2v = new PacketTagF2v();
	  pktTagF2v->SetCurrentEdgeType(EdgeType::CONDITION_1);
	  pktTagF2v->SetNextActionType(PacketTagF2v::NextActionType::NOT_SET);
	  pktTagF2v->SetSrcNodeId(nodeId);
	  pktTagF2v->SetDataIdxs(pktTagF2f.GetDataIdxs());
	  sender->SetPacketTag(pktTagF2v);

	  Simulator::ScheduleNow (&UdpSender::Send, sender);
	}
      else if (edgeType == EdgeType::CONDITION_3)
	{
	  Ptr<UdpSender> sender = CreateObject<UdpSender>();
	  sender->SetNode(NodeList::GetNode(nodeId));
	  sender->SetRemote(Ipv4Address ("10.3.255.255"), m_i2VPort);
	  sender->SetDataSize(Packet_Size);
	  sender->Start();
	  using vanet::PacketHeader;
	  PacketHeader header;
	  header.SetType(PacketHeader::MessageType::DATA_F2V);
	  header.SetBroadcastId(broadcastId);
	  sender->SetHeader(header);

	  using vanet::PacketTagF2v;
	  PacketTagF2v *pktTagF2v = new PacketTagF2v();
	  pktTagF2v->SetCurrentEdgeType(EdgeType::CONDITION_3);
	  pktTagF2v->SetNextActionType(PacketTagF2v::NextActionType::NOT_SET);
	  pktTagF2v->SetSrcNodeId(nodeId);
	  pktTagF2v->SetDataIdxs(pktTagF2f.GetDataIdxs());
	  sender->SetPacketTag(pktTagF2v);

	  Simulator::ScheduleNow (&UdpSender::Send, sender);
	}
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_F2V)
    {
      using vanet::PacketTagF2v;
      PacketTagF2v pktTagF2v;
      pktCopy->RemovePacketTag(pktTagF2v);

      uint32_t fogId = pktTagF2v.GetSrcNodeId();
      Ptr<Node> rsu = NodeList::GetNode(fogId);
      Ptr<MobilityModel> rsuMobility = rsu->GetObject<MobilityModel> ();
      Vector pos_rsu = rsuMobility->GetPosition ();

      Ptr<Node> obu = NodeList::GetNode(nodeId);
      Ptr<MobilityModel> obuMobility = obu->GetObject<MobilityModel> ();
      Vector pos_obu = obuMobility->GetPosition ();

      if (CalculateDistance (pos_obu, pos_rsu) > m_rsuTxRange )
	{
	  return;
	}

      uint32_t fogIdx = m_fogId2FogIdxMap.at(pktTagF2v.GetSrcNodeId());
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);
      EdgeType edgeType = pktTagF2v.GetCurrentEdgeType();
      PacketTagF2v::NextActionType nextAction = pktTagF2v.GetNextActionType();

      PrintEdgeType(edgeType, oss);

      if (edgeType == EdgeType::NOT_SET)
	{
	  std::vector<uint32_t> dataIdxs = pktTagF2v.GetDataIdxs();
	  for (uint32_t dataIdx : dataIdxs)
	    {
	      map<uint32_t, std::set<uint32_t>>::iterator iter = m_extraDataForDecoding[obuIdx].find(broadcastId);
//	      NS_ASSERT (iter != datasNeededForDecodingPerClique[obuIdx].end());
	      if (iter != m_extraDataForDecoding[obuIdx].end())
		{
		  iter->second.erase(dataIdx);
		  if (iter->second.size() == 0)
		    {
		      if (m_fogIdx2FogReqInCliqueMaps.count(iter->first)
			  && m_fogIdx2FogReqInCliqueMaps.at(iter->first).count(fogIdx))
			{
			  uint32_t fogReqIdx = m_fogIdx2FogReqInCliqueMaps.at(iter->first).at(fogIdx);
			  if (m_vehsReqs[obuIdx].count(fogReqIdx) != 0)
			    {
			      m_vehsReqs[obuIdx].erase(fogReqIdx);
			      m_vehsCaches[obuIdx].insert(fogReqIdx);
			      DoStats(obuIdx, fogReqIdx);
			    }
			}
		    }
		}

	      if (m_vehsReqs[obuIdx].count(dataIdx) != 0)
		{
		  m_vehsReqs[obuIdx].erase(dataIdx);
		  m_vehsCaches[obuIdx].insert(dataIdx);
		  DoStats(obuIdx, dataIdx);
		}
	    }
	}
      else if (edgeType == EdgeType::CONDITION_1)
	{
	  std::vector<uint32_t> dataIdxs = pktTagF2v.GetDataIdxs();
	  for (uint32_t dataIdx : dataIdxs)
	    {
	      if (m_vehsReqs[obuIdx].count(dataIdx) != 0)
		{
//		  vehsCaches[obuIdx].insert(dataIdx);
//		  map<uint32_t, std::set<uint32_t>>::iterator iter = datasNeededForDecodingPerClique[obuIdx].begin();
//		  for (; iter != datasNeededForDecodingPerClique[obuIdx].end(); iter++)
//		    {
//		      iter->second.erase(dataIdx);
//		      if (iter->second.size() == 0)
//			{
//			  if (fogIdx2FogReqInCliqueMaps.count(iter->first)
//			      && fogIdx2FogReqInCliqueMaps.at(iter->first).count(fogIdx))
//			    {
//			      uint32_t fogReqIdx = fogIdx2FogReqInCliqueMaps.at(iter->first).at(fogIdx);
//			      if (vehsReqs[obuIdx].count(fogReqIdx) != 0)
//				{
//				  vehsReqs[obuIdx].erase(fogReqIdx);
//				  vehsCaches[obuIdx].insert(fogReqIdx);
//				  RecordStats(obuIdx, fogReqIdx);
//				}
//			    }
//			}
//		    }

		  m_vehsReqs[obuIdx].erase(dataIdx);
		  DoStats(obuIdx, dataIdx);

		  m_vehsCaches[obuIdx].insert(dataIdx);
		  map<uint32_t, std::set<uint32_t>>::iterator iter = m_extraDataForDecoding[obuIdx].find(broadcastId);
//		  NS_ASSERT (iter != datasNeededForDecodingPerClique[obuIdx].end());
		  if (iter != m_extraDataForDecoding[obuIdx].end())
		    {
		      iter->second.erase(dataIdx);
		      if (iter->second.size() == 0)
			{
			  if (m_fogIdx2FogReqInCliqueMaps.count(iter->first)
			      && m_fogIdx2FogReqInCliqueMaps.at(iter->first).count(fogIdx))
			    {
			      uint32_t fogReqIdx = m_fogIdx2FogReqInCliqueMaps.at(iter->first).at(fogIdx);
			      if (m_vehsReqs[obuIdx].count(fogReqIdx) != 0)
				{
				  m_vehsReqs[obuIdx].erase(fogReqIdx);
				  m_vehsCaches[obuIdx].insert(fogReqIdx);
				  DoStats(obuIdx, fogReqIdx);
				}
			    }
			}
		    }
		}
	    }
	}
      else if (edgeType == EdgeType::CONDITION_2)
	{
	  if (m_fogIdx2FogReqInCliqueMaps.count(broadcastId) == 0) return;
	  std::vector<uint32_t> dataIdxs = pktTagF2v.GetDataIdxs();
	  for (uint32_t dataIdx : dataIdxs)
	    {

//	      vehsCaches[obuIdx].insert(dataIdx);
//	      map<uint32_t, std::set<uint32_t>>::iterator iter = datasNeededForDecodingPerClique[obuIdx].begin();
//	      for (; iter != datasNeededForDecodingPerClique[obuIdx].end(); iter++)
//		{
//		  iter->second.erase(dataIdx);
//		  if (iter->second.size() == 0)
//		    {
//		      if (fogIdx2FogReqInCliqueMaps.count(iter->first)
//			  && fogIdx2FogReqInCliqueMaps.at(iter->first).count(fogIdx))
//			{
//			  uint32_t fogReqIdx = fogIdx2FogReqInCliqueMaps.at(iter->first).at(fogIdx);
//			  if (vehsReqs[obuIdx].count(fogReqIdx) != 0)
//			    {
//			      vehsReqs[obuIdx].erase(fogReqIdx);
//			      vehsCaches[obuIdx].insert(fogReqIdx);
//			      RecordStats(obuIdx, fogReqIdx);
//			    }
//			}
//		    }
//		}
//	      if (vehsReqs[obuIdx].count(dataIdx) != 0)
//		{
//		  vehsReqs[obuIdx].erase(dataIdx);
//		  vehsCaches[obuIdx].insert(dataIdx);
//		  RecordStats(obuIdx, dataIdx);
//		}

	      map<uint32_t, std::set<uint32_t>>::iterator iter = m_extraDataForDecoding[obuIdx].find(broadcastId);
//	      NS_ASSERT (iter != datasNeededForDecodingPerClique[obuIdx].end());
	      if (iter != m_extraDataForDecoding[obuIdx].end())
		{
		  iter->second.erase(dataIdx);
		  if (iter->second.size() == 0)
		    {
		      if (m_fogIdx2FogReqInCliqueMaps.count(iter->first)
			  && m_fogIdx2FogReqInCliqueMaps.at(iter->first).count(fogIdx))
			{
			  uint32_t fogReqIdx = m_fogIdx2FogReqInCliqueMaps.at(iter->first).at(fogIdx);
			  if (m_vehsReqs[obuIdx].count(fogReqIdx) != 0)
			    {
			      m_vehsReqs[obuIdx].erase(fogReqIdx);
			      m_vehsCaches[obuIdx].insert(fogReqIdx);
			      DoStats(obuIdx, fogReqIdx);
			    }
			}
		    }
		}

	      if (m_vehsReqs[obuIdx].count(dataIdx) != 0)
		{
		  m_vehsReqs[obuIdx].erase(dataIdx);
		  m_vehsCaches[obuIdx].insert(dataIdx);
		  DoStats(obuIdx, dataIdx);
		}
	    }
	}
      else if (edgeType == EdgeType::CONDITION_3)
	{
	  if (m_fogIdx2FogReqInCliqueMaps.count(broadcastId) == 0) return;
	  std::vector<uint32_t> dataIdxs = pktTagF2v.GetDataIdxs();
	  for (uint32_t dataIdx : dataIdxs)
	    {
//	      vehsCaches[obuIdx].insert(dataIdx);
//	      map<uint32_t, std::set<uint32_t>>::iterator iter = datasNeededForDecodingPerClique[obuIdx].begin();
//	      for (; iter != datasNeededForDecodingPerClique[obuIdx].end(); iter++)
//		{
//		  iter->second.erase(dataIdx);
//		  if (iter->second.size() == 0)
//		    {
//		      if (fogIdx2FogReqInCliqueMaps.count(iter->first)
//			  && fogIdx2FogReqInCliqueMaps.at(iter->first).count(fogIdx))
//			{
//			  uint32_t fogReqIdx = fogIdx2FogReqInCliqueMaps.at(iter->first).at(fogIdx);
//			  if (vehsReqs[obuIdx].count(fogReqIdx) != 0)
//			    {
//			      vehsReqs[obuIdx].erase(fogReqIdx);
//			      vehsCaches[obuIdx].insert(fogReqIdx);
//			      RecordStats(obuIdx, fogReqIdx);
//			    }
//			}
//		    }
//		}
//	      if (vehsReqs[obuIdx].count(dataIdx) != 0)
//		{
//		  vehsReqs[obuIdx].erase(dataIdx);
//		  RecordStats(obuIdx, dataIdx);
//		}

	      map<uint32_t, std::set<uint32_t>>::iterator iter = m_extraDataForDecoding[obuIdx].find(broadcastId);
//	      NS_ASSERT (iter != datasNeededForDecodingPerClique[obuIdx].end());

	      bool decodeSuccess = false;
	      uint32_t fogReqIdx;
	      if (iter != m_extraDataForDecoding[obuIdx].end())
		{
		  iter->second.erase(dataIdx);
		  if (iter->second.size() == 0)
		    {
		      if (m_fogIdx2FogReqInCliqueMaps.count(iter->first)
			  && m_fogIdx2FogReqInCliqueMaps.at(iter->first).count(fogIdx))
			{
			  fogReqIdx = m_fogIdx2FogReqInCliqueMaps.at(iter->first).at(fogIdx);
			  if (m_vehsReqs[obuIdx].count(fogReqIdx) != 0)
			    {
			      decodeSuccess = true;
			      m_vehsReqs[obuIdx].erase(fogReqIdx);
			      m_vehsCaches[obuIdx].insert(fogReqIdx);
			      DoStats(obuIdx, fogReqIdx);
			    }
			}
		    }
		}

	      if (m_vehsReqs[obuIdx].count(dataIdx) != 0)
		{
		  m_vehsReqs[obuIdx].erase(dataIdx);
		  m_vehsCaches[obuIdx].insert(dataIdx);
		  DoStats(obuIdx, dataIdx);
		}

	      if (nextAction == PacketTagF2v::NextActionType::V2F)
		{
		  if (pktTagF2v.GetRsuWaitingServedIdxs().size() != 0)
		    {
		      Ptr<UdpSender> sender = CreateObject<UdpSender>();
		      sender->SetNode(NodeList::GetNode(nodeId));
		      sender->SetRemote(m_rsu80211pInterfaces.GetAddress(fogIdx), m_v2IPort);
		      sender->SetDataSize(Packet_Size);
		      sender->Start();
		      using vanet::PacketHeader;
		      PacketHeader header;
		      header.SetType(PacketHeader::MessageType::DATA_V2F);
		      header.SetBroadcastId(broadcastId);
		      sender->SetHeader(header);

		      using vanet::PacketTagV2f;
		      PacketTagV2f *pktTagV2f = new PacketTagV2f();
		      pktTagV2f->SetCurrentEdgeType(EdgeType::CONDITION_3);
		      pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2F);
		      pktTagV2f->SetNextRsuIdxs(pktTagF2v.GetRsuWaitingServedIdxs());
		      std::vector<uint32_t> dataIdxsToSend;
		      dataIdxsToSend.assign(dataIdxs.begin(), dataIdxs.end());
		      if (decodeSuccess)
			{
			  dataIdxsToSend.push_back(fogReqIdx);
			}
		      pktTagV2f->SetDataIdxs(dataIdxsToSend);
		      sender->SetPacketTag(pktTagV2f);

		      Simulator::ScheduleNow (&UdpSender::Send, sender);
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

  oss << " bId: " << broadcastId;

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
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);

      // Judging whether the vehicle is in the service area or not
      if (!m_vehsEnter[obuIdx]) return;

      using vanet::PacketTagC2v;
      PacketTagC2v pktTagC2v;
      pktCopy->RemovePacketTag(pktTagC2v);
      std::vector<uint32_t> datasIdxBroadcast = pktTagC2v.GetReqsIds();
      uint32_t broadcastDataNum = datasIdxBroadcast.size();

      if (broadcastDataNum == 0) return;

      // Judging whether the broadcasted data is encoded or not
      bool isEncoded = broadcastDataNum == 1 ? false : true;
      oss << " Encoded: " << isEncoded;
      if (!isEncoded)
	{
	  uint32_t dataIdx = datasIdxBroadcast[0];
	  if (m_vehsToSatisfy[broadcastId].count(obuIdx))
	    {
	      if (m_vehsReqs[obuIdx].count(dataIdx))
		{
		  m_vehsCaches[obuIdx].insert(dataIdx);
		  m_vehsReqs[obuIdx].erase(dataIdx);
		  DoStats(obuIdx, dataIdx);

		  ReqQueueItem item(obuIdx, dataIdx);
		  m_requestQueue.erase(std::find(m_requestQueue.begin(), m_requestQueue.end(), item));

		  m_requestsToMarkWithBid[broadcastId].erase(item.name);
		}
	    }
	}
      else
	{
	  if (m_vehsToSatisfy[broadcastId].count(obuIdx))
	    {
	      uint32_t dataIdx1 = datasIdxBroadcast[0];
	      uint32_t dataIdx2 = datasIdxBroadcast[1];
//	      if (broadcastId == Test_Bid)
//		{
//		  cout << "bid:" << broadcastId << ", obuid:" << obuIdx << ", data1:" << dataIdx1 << ", data2:" << dataIdx2 << endl;
//		}

	      std::ostringstream oss1;
	      oss1 << obuIdx << "-" << dataIdx1;

	      std::ostringstream oss2;
	      oss2 << obuIdx << "-" << dataIdx2;

	      if (m_requestsToMarkWithBid[broadcastId].count(oss1.str()))
		{
		  if (m_vehsReqs[obuIdx].count(dataIdx1) && m_vehsCaches[obuIdx].count(dataIdx2))
		    {
		      m_vehsCaches[obuIdx].insert(dataIdx1);
		      m_vehsReqs[obuIdx].erase(dataIdx1);
		      DoStats(obuIdx, dataIdx1);

		      ReqQueueItem item(obuIdx, dataIdx1);
		      m_requestQueue.erase(std::find(m_requestQueue.begin(), m_requestQueue.end(), item));

		      m_requestsToMarkWithBid[broadcastId].erase(item.name);

//		      if (broadcastId == Test_Bid)
//			{
//			  cout << 1 << endl;
//			}
		    }
		  else if (m_vehsReqs[obuIdx].count(dataIdx1) && m_vehsCaches[obuIdx].count(dataIdx2) == 0)
		    {
//		      if (broadcastId == Test_Bid)
//			{
//			  cout << 3;
//			}
		      bool flag = true;
		      if (m_vehIdx2FogIdxMap.count(obuIdx))
			{
			  uint32_t fogIdx = m_vehIdx2FogIdxMap.at(obuIdx);
			  if (m_fogsCaches[fogIdx].count(dataIdx2))
			    {
			      for (uint32_t fogVehIdx : m_fogCluster[fogIdx])
				{
				  if (m_vehsCaches[fogVehIdx].count(dataIdx2))
				    {
//				      if (broadcastId == Test_Bid)
//					{
//					  cout << " true" << endl;
//					}
				      Ptr<UdpSender> sender = CreateObject<UdpSender>();
				      sender->SetNode(m_obuNodes.Get(fogVehIdx));
				      sender->SetRemote(m_rsu80211pInterfaces.GetAddress(fogIdx), m_v2IPort);
				      sender->SetDataSize(Packet_Size);
				      sender->Start();
				      using vanet::PacketHeader;
				      PacketHeader header;
				      header.SetType(PacketHeader::MessageType::DATA_V2F);
				      header.SetBroadcastId(broadcastId);
				      sender->SetHeader(header);

				      using vanet::PacketTagV2f;
				      PacketTagV2f *pktTagV2f = new PacketTagV2f();
				      pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2V);
				      std::vector<uint32_t> dataIdxs;
				      dataIdxs.push_back(dataIdx2);
				      pktTagV2f->SetDataIdxs(dataIdxs);
				      sender->SetPacketTag(pktTagV2f);

				      Simulator::ScheduleNow (&UdpSender::Send, sender);
				      break;
				    }
				}
			    }
			  else
			    {
			      flag = false;
			    }
			}
		      else
			{
			  flag = false;
			}
		      if (!flag)
			{
			  std::ostringstream oss;
			  oss << obuIdx << "-" << dataIdx1;
			  m_requestsToMarkGloabal[oss.str()] = false;
			}
		    }
//		  else
//		    {
//		      if (broadcastId == Test_Bid)
//			{
//			  cout << 6 << endl;
//			}
//		    }
		}
	      else if (m_requestsToMarkWithBid[broadcastId].count(oss2.str()))
		{
		  if (m_vehsReqs[obuIdx].count(dataIdx2) && m_vehsCaches[obuIdx].count(dataIdx1))
		    {
		      m_vehsCaches[obuIdx].insert(dataIdx2);
		      m_vehsReqs[obuIdx].erase(dataIdx2);
		      DoStats(obuIdx, dataIdx2);

		      ReqQueueItem item(obuIdx, dataIdx2);
		      m_requestQueue.erase(std::find(m_requestQueue.begin(), m_requestQueue.end(), item));

		      m_requestsToMarkWithBid[broadcastId].erase(item.name);

//		      if (broadcastId == Test_Bid)
//			{
//			  cout << 2 << endl;
//			}
		    }
		  else if (m_vehsReqs[obuIdx].count(dataIdx2) && m_vehsCaches[obuIdx].count(dataIdx1) == 0)
		    {
//		      if (broadcastId == Test_Bid)
//			{
//			  cout << 4;
//			}
		      bool flag = true;
		      if (m_vehIdx2FogIdxMap.count(obuIdx))
			{
			  uint32_t fogIdx = m_vehIdx2FogIdxMap.at(obuIdx);
			  if (m_fogsCaches[fogIdx].count(dataIdx1))
			    {
			      for (uint32_t fogVehIdx : m_fogCluster[fogIdx])
				{
				  if (m_vehsCaches[fogVehIdx].count(dataIdx1))
				    {
//				      if (broadcastId == Test_Bid)
//					{
//					  cout << " true" << endl;
//					}
				      Ptr<UdpSender> sender = CreateObject<UdpSender>();
				      sender->SetNode(m_obuNodes.Get(fogVehIdx));
				      sender->SetRemote(m_rsu80211pInterfaces.GetAddress(fogIdx), m_v2IPort);
				      sender->SetDataSize(Packet_Size);
				      sender->Start();
				      using vanet::PacketHeader;
				      PacketHeader header;
				      header.SetType(PacketHeader::MessageType::DATA_V2F);
				      header.SetBroadcastId(broadcastId);
				      sender->SetHeader(header);

				      using vanet::PacketTagV2f;
				      PacketTagV2f *pktTagV2f = new PacketTagV2f();
				      pktTagV2f->SetNextActionType(PacketTagV2f::NextActionType::F2V);
				      std::vector<uint32_t> dataIdxs;
				      dataIdxs.push_back(dataIdx1);
				      pktTagV2f->SetDataIdxs(dataIdxs);
				      sender->SetPacketTag(pktTagV2f);

				      Simulator::ScheduleNow (&UdpSender::Send, sender);
				      break;
				    }
				}
			    }
			  else
			    {
			      flag = false;
			    }
			}
		      else
			{
			  flag = false;
			}
		      if (!flag)
			{
			  std::ostringstream oss;
			  oss << obuIdx << "-" << dataIdx2;
			  m_requestsToMarkGloabal[oss.str()] = false;
			}
		    }
//		  else
//		    {
//		      if (broadcastId == Test_Bid)
//			{
//			  cout << 7 << endl;
//			}
//		    }
		}
//	      else
//		{
//		  if (broadcastId == Test_Bid)
//		    {
//		      cout << 5 << endl;
//		    }
//		}
	    }
	}

//      dataToBroadcast.erase(broadcastId);
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_V2F)
    {
      using vanet::PacketTagV2f;
      PacketTagV2f pktTagV2f;
      pktCopy->RemovePacketTag(pktTagV2f);

      Ptr<UdpSender> sender = CreateObject<UdpSender>();
      sender->SetNode(NodeList::GetNode(nodeId));
      sender->SetRemote(Ipv4Address ("10.3.255.255"), m_i2VPort);
      sender->SetDataSize(Packet_Size);
      sender->Start();
      using vanet::PacketHeader;
      PacketHeader header;
      header.SetType(PacketHeader::MessageType::DATA_F2V);
      header.SetBroadcastId(broadcastId);
      sender->SetHeader(header);

      using vanet::PacketTagF2v;
      PacketTagF2v *pktTagF2v = new PacketTagF2v();
      pktTagF2v->SetNextActionType(PacketTagF2v::NextActionType::NOT_SET);
      pktTagF2v->SetSrcNodeId(nodeId);
      pktTagF2v->SetDataIdxs(pktTagV2f.GetDataIdxs());
      sender->SetPacketTag(pktTagF2v);

      Simulator::ScheduleNow (&UdpSender::Send, sender);
    }
  else if (recvHeader.GetType() == PacketHeader::MessageType::DATA_F2V)
    {
      using vanet::PacketTagF2v;
      PacketTagF2v pktTagF2v;
      pktCopy->RemovePacketTag(pktTagF2v);

      uint32_t fogId = pktTagF2v.GetSrcNodeId();
      Ptr<Node> rsu = NodeList::GetNode(fogId);
      Ptr<MobilityModel> rsuMobility = rsu->GetObject<MobilityModel> ();
      Vector pos_rsu = rsuMobility->GetPosition ();

      Ptr<Node> obu = NodeList::GetNode(nodeId);
      Ptr<MobilityModel> obuMobility = obu->GetObject<MobilityModel> ();
      Vector pos_obu = obuMobility->GetPosition ();

      if (CalculateDistance (pos_obu, pos_rsu) > m_rsuTxRange )
	{
	  return;
	}

      std::vector<uint32_t> datasIdx = pktTagF2v.GetDataIdxs();
      uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);
      std::vector<uint32_t> dataBroadcast =  m_dataToBroadcast[broadcastId];

      if (m_vehsToSatisfy[broadcastId].count(obuIdx))
	{
	  uint32_t dataIdx = datasIdx[0];


	  if (dataBroadcast[1] == dataIdx)
	    {
	      std::ostringstream oss;
	      oss << obuIdx << "-" << dataBroadcast[0];
	      if (m_requestsToMarkWithBid[broadcastId].count(oss.str()))
		{
		  if (m_vehsReqs[obuIdx].count(dataBroadcast[0]))
		    {
		      m_vehsCaches[obuIdx].insert(dataBroadcast[0]);
		      m_vehsReqs[obuIdx].erase(dataBroadcast[0]);
		      DoStats(obuIdx, dataBroadcast[0]);

		      ReqQueueItem item(obuIdx, dataBroadcast[0]);
		      m_requestQueue.erase(std::find(m_requestQueue.begin(), m_requestQueue.end(), item));

		      m_requestsToMarkWithBid[broadcastId].erase(item.name);
		    }
		}
	    }
	  else if (dataBroadcast[0] == dataIdx)
	    {
	      std::ostringstream oss;
	      oss << obuIdx << "-" << dataBroadcast[1];
	      if (m_requestsToMarkWithBid[broadcastId].count(oss.str()))
		{
		  if (m_vehsReqs[obuIdx].count(dataBroadcast[1]))
		    {
		      m_vehsCaches[obuIdx].insert(dataBroadcast[1]);
		      m_vehsReqs[obuIdx].erase(dataBroadcast[1]);
		      DoStats(obuIdx, dataBroadcast[1]);

		      ReqQueueItem item(obuIdx, dataBroadcast[1]);
		      m_requestQueue.erase(std::find(m_requestQueue.begin(), m_requestQueue.end(), item));

		      m_requestsToMarkWithBid[broadcastId].erase(item.name);
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
VanetCsVfcExperiment::ReceivePacketOnSchemeMA (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr)
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

  uint32_t obuIdx = m_vehId2IndexMap.at(nodeId);

  // Judging whether the vehicle is in the service area or not
  if (!m_vehsEnter[obuIdx]) return;

  using vanet::PacketHeader;
  PacketHeader recvHeader;
  Ptr<Packet> pktCopy = packet->Copy();
  pktCopy->RemoveHeader(recvHeader);

  uint32_t broadcastId = recvHeader.GetBroadcastId();

  oss << " bId: " << broadcastId;

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
      using vanet::PacketTagC2v;
      PacketTagC2v pktTagC2v;
      pktCopy->RemovePacketTag(pktTagC2v);
      std::vector<uint32_t> datasIdxBroadcast = pktTagC2v.GetReqsIds();
      uint32_t broadcastDataNum = datasIdxBroadcast.size();

      if (broadcastDataNum == 0) return;

      mwArray vehCacheMatrix(m_globalDbSize, m_globalDbSize, mxDOUBLE_CLASS);
      for (uint32_t i = 1; i <= m_globalDbSize; i++)
	{
	  for (uint32_t j = 1; j <= m_globalDbSize; j++)
	    {
	      vehCacheMatrix(i, j) = (*mwVehsCachesMatrix)(i, j, obuIdx + 1);
	    }
	}

      mwArray mwDatasIdxBroadcast(1, m_globalDbSize, mxDOUBLE_CLASS);
      for (uint32_t dataIdx : datasIdxBroadcast)
	{
	  mwDatasIdxBroadcast(1, dataIdx + 1) = 1.0;
	}

//      cout << "vehCacheMatrix[" << obuIdx + 1 << "]:" << endl;
//      for (uint32_t i = 1; i <= globalDbSize; i++)
//	{
//	  for (uint32_t j = 1; j <= globalDbSize; j++)
//	    {
//	      cout << " " << vehCacheMatrix(i, j);
//	    }
//	  cout << endl;
//	}
//      cout << "mwDatasIdxBroadcast:";
//      for (uint32_t i = 1; i <= globalDbSize; i++)
//	{
//	  cout << " " << mwDatasIdxBroadcast(1, i);
//	}
//      cout << endl;


      mwArray mwResult1(mxUINT32_CLASS);
      mwArray mwResult2(mxUINT32_CLASS);
      mwArray mwResult3(mxDOUBLE_CLASS);
      MADecode(3, mwResult1, mwResult2, mwResult3, vehCacheMatrix, mwDatasIdxBroadcast);
      uint32_t flag = mwResult1(1, 1);
      if (flag > 0)
	{
	  for (uint32_t i = 1; i <= flag; i++)
	    {
	      uint32_t dataToCached = mwResult2(1, i);
	      dataToCached -= 1; // the data index in matlab starts from 1

	      std::cout << "obuIdx " << obuIdx << " cache data " << dataToCached << std::endl;

	      m_vehsCaches[obuIdx].insert(dataToCached);
	      m_vehsReqs[obuIdx].erase(dataToCached);
	      DoStats(obuIdx, dataToCached);
	    }
	}

      for (uint32_t i = 1; i <= m_globalDbSize; i++)
	{
	  for (uint32_t j = 1; j <= m_globalDbSize; j++)
	    {
	      (*mwVehsCachesMatrix)(i, j, obuIdx + 1) = mwResult3(i, j);
	    }
	}
    }

  oss << " DbSize:" << m_globalDbSize;
#if Print_Received_Log
  NS_LOG_UNCOND(oss.str());
#endif
}

void
VanetCsVfcExperiment::OnOffTrace (std::string context, Ptr<const Packet> packet)
{
  uint32_t pktBytes = packet->GetSize ();
  m_routingStats.IncTxBytes (pktBytes);
}

void
VanetCsVfcExperiment::Initialization ()
{
#if Three_By_Three
  m_obuTxp = 137.0; //450m
  m_rsuTxp = 137.0;
  m_bsTxp = 166.1; // 2122m
  m_rsuTxRange = 450;

  m_mobilityFile = m_cwd + "/mobility" + "/mobility-3x3.tcl";
#else
  if (m_nRsuNodes == 16)
    {
      m_obuTxp = 128; //400m
      m_rsuTxp = 128;
      m_rsuTxRange = 400;
    }
  else if (m_nRsuNodes == 9)
    {
      m_obuTxp = 140.5; //525m
      m_rsuTxp = 140.5;
      m_rsuTxRange = 525;
    }

  m_bsTxp = 164.2; // 1770m
  m_bsTxRange = 1770;

  m_mobilityFile = m_cwd + "/mobility" + "/mobility-jiangbei.tcl";
#endif

  m_mobLogFile = m_cwd + "/mobility.log";
  m_trName = m_cwd + "/vanet-cs-vfc.mob";
  m_animFile = m_cwd + "/vanet-cs-vfc-animation.xml";

  m_wifiPhyStats = CreateObject<WifiPhyStats> ();

  m_globalDB.resize(m_globalDbSize);
  m_vehsEnter.resize(m_nObuNodes, false);
  m_nodesMoving.resize(m_nObuNodes, false);

  m_vehsInitialReqs.resize(m_nObuNodes);
  m_vehsInitialCaches.resize(m_nObuNodes);
  m_vehsReqs.resize(m_nObuNodes);
  m_vehsReqsStatus.resize(m_nObuNodes);
  m_vehsCaches.resize(m_nObuNodes);

  m_vehsReqsInCloud.resize(m_nObuNodes);
  m_vehsReqsStasInCloud.resize(m_nObuNodes);
  m_vehsCachesInCloud.resize(m_nObuNodes);
  m_vehsMobInfoInCloud.resize(m_nObuNodes);

  m_fogCluster.resize(m_nRsuNodes);
  m_fogsCaches.resize(m_nRsuNodes);
  m_fogsReqs.resize(m_nRsuNodes);

  m_isFirstSubmit.resize(m_nObuNodes, true);

  m_extraDataForDecoding.resize(m_nObuNodes);

  // Initializing global data base
  for (uint32_t i = 0; i < m_globalDbSize; i++)
    {
      m_globalDB[i] = i;
    }

  // Initializing vehicle cache and request set
#if 1
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
#endif

#if 0
  // for testing
  for (uint32_t i = 0; i < m_nObuNodes; i++)
    {

      std::set<uint32_t> vehReq;
      std::set<uint32_t> vehCache(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(1);
      vehCache.erase(1);
//      vehReq.insert(2);
//      vehCache.erase(2);
      m_vehsInitialCaches[1] = vehCache;
      m_vehsInitialReqs[1] = vehReq;

      vehReq.clear();
      vehCache.clear();
      vehCache.insert(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(2);
      vehCache.erase(2);
      m_vehsInitialCaches[2] = vehCache;
      m_vehsInitialReqs[2] = vehReq;

      vehReq.clear();
      vehCache.clear();
      vehCache.insert(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(3);
      vehCache.erase(3);
      m_vehsInitialCaches[3] = vehCache;
      m_vehsInitialReqs[3] = vehReq;

      vehReq.clear();
      vehCache.clear();
      vehCache.insert(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(4);
      vehCache.erase(4);
      m_vehsInitialCaches[4] = vehCache;
      m_vehsInitialReqs[4] = vehReq;

      vehReq.clear();
      vehCache.clear();
      vehCache.insert(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(5);
      vehCache.erase(5);
      m_vehsInitialCaches[5] = vehCache;
      m_vehsInitialReqs[5] = vehReq;

      vehReq.clear();
      vehCache.clear();
      vehCache.insert(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(6);
      vehCache.erase(6);
      m_vehsInitialCaches[20] = vehCache;
      m_vehsInitialReqs[20] = vehReq;

      vehReq.clear();
      vehCache.clear();
      vehCache.insert(m_globalDB.begin(), m_globalDB.end());
      vehReq.insert(7);
      vehCache.erase(7);
      m_vehsInitialCaches[21] = vehCache;
      m_vehsInitialReqs[21] = vehReq;

    }
#endif

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
  if (m_schemeName.compare(Scheme_3) == 0)
    {
      if(!libMAInitialize())
      {
	std::cout << "Could not initialize libMA!" << std::endl;
	return;
      }
      if(!libMADecodeInitialize())
      {
	std::cout << "Could not initialize libMADecode!" << std::endl;
	return;
      }

      mwSize dims[3] = {m_globalDbSize, m_globalDbSize, m_nObuNodes};
      mwVehsCachesMatrix = new mwArray(3, dims, mxDOUBLE_CLASS);
      for (uint32_t obuIdx = 0; obuIdx < m_nObuNodes; obuIdx++)
        {
	  uint32_t k = 1;
          for (uint32_t i = 0; i < m_globalDbSize; i++)
	    {
	      if (m_vehsInitialCaches[obuIdx].count(i))
		{
		  (*mwVehsCachesMatrix)(k, i + 1, obuIdx + 1) = 1.0;
		  k += 1;
		}
	    }
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
