/*
 * vanet-facs.h
 *
 *  Created on: Jul 26, 2018
 *      Author: Yang yanning <yang.ksn@gmail.com>
 */

#ifndef SCRATCH_VANET_CS_VFC_VANET_CS_VFC_H_
#define SCRATCH_VANET_CS_VFC_VANET_CS_VFC_H_

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <time.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wave-module.h"
#include "ns3/csma-module.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/gnuplot.h"
#include "graph.hpp"
#include "udp-sender.h"
#include "data-tx-application.h"
#include "my-point-to-point-star.h"
#include "byte-buffer.h"
#include "packet-header.h"
#include "packet-tag-c2v.h"
#include "packet-tag-v2f.h"
#include "packet-tag-f2f.h"
#include "packet-tag-f2v.h"
#include "packet-tag-v2v.h"
#include "stats.h"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mclmcrrt.h>
#include <mclcppclass.h>
#include <matrix.h>
#include "libFACS.h"
#include "libNCB.h"
#include "libISXD.h"
#include "libZipf_Request.h"

NS_LOG_COMPONENT_DEFINE ("vanet-facs");

#define Gen_Animation 						false
#define Gen_Gnuplot_File					false

#define Lte_Enable 						false	// turn on/off LTE
#define Friis_Propagation_Loss_Model 				false
#define Range_Propagation_Loss_Model 				true
#define Upload_Enable 						false
#define Cloud_Enable 						true
#define Print_Log_Header_On_Receive 				true
#define Print_Msg_Type 						true
#define Print_Edge_Type 					true
#define Print_Received_Data_Cloud 				false
#define Print_Received_Log 					false
#define Print_Received_Data 					true
#define Print_Vehicle_Initial_Request 				false
#define Print_Vehicle_Initial_Cache 				false
#define Print_Vehicle_Request 					false
#define Print_Vehicle_Cache 					false
#define Print_Vehicle_Final_Request 				false
#define Print_Vehicle_Final_Cache 				false
#define Print_Count_Of_Veh_In_Area 				false
#define Print_Fog_Cluster 					false
#define Print_Distance_OBU_RSU 					false
#define Print_Total_Num_Veh_In_Fog 				false
#define Print_Remaining_Time 					false
#define Scheduling 						true
#define Print_Edge 						false
#define Search_Clique 						true
#define Print_Cliques 						true
#define Total_Time_Spent_stas 					false
#define Construct_Graph_And_Find_Clique_Time_stas 		false

#define Loop_Scheduling						true

const string Scheme_1 = "facs";
const string Scheme_2 = "ncb";
const string Scheme_3 = "isxd";

const string PROTOCOL_UDP = "ns3::UdpSocketFactory";
const string PROTOCOL_TCP = "ns3::TcpSocketFactory";

const double Rate_F2F = 200;
const double Rate_V2V = 12;
const double Rate_V2F = 6;
const double Rate_C2V = 3;
const double PhyRate_V2V = 12;
const double PhyRate_V2F = 6;
const double PhyRate_C2V = 3;
const double PhyRate_Max = 27;

const double Data_Recv_Rate = 0.9;
const double DelayDelta = 0.1;

const uint32_t Zipf_Min_Req = 15;
const uint32_t Zipf_Max_Req = 25;
const double Zipf_Theta = 0.4;

class VanetCsVfcExperiment
{
public:
  /**
   * \brief Constructor
   * \return none
   */
  VanetCsVfcExperiment ();

  /**
   * \brief destructor
   */
  ~VanetCsVfcExperiment ();

  /**
   * \brief Trace the receipt of an on-off-application generated packet
   * \param context this object
   * \param packet a received packet
   * \return none
   */
  void OnOffTrace (std::string context, Ptr<const Packet> packet);

  /**
   * \brief Enacts simulation of an ns-3 wifi application
   * \param argc program arguments count
   * \param argv program arguments
   * \return none
   */
  void Simulate (int argc, char **argv);

  int MkPath(std::string s,mode_t mode);

protected:
  /**
   * \brief Sets default attribute values
   * \return none
   */
  virtual void SetDefaultAttributeValues ();

  /**
   * \brief Process command line arguments
   * \param argc program arguments count
   * \param argv program arguments
   * \return none
   */
  virtual void ParseCommandLineArguments (int argc, char **argv);

  /**
   * \brief Configure nodes
   * \return none
   */
  virtual void ConfigureNodes ();

  /**
   * \brief Configure channels
   * \return none
   */
  virtual void ConfigureChannels ();

  /**
   * \brief Configure devices
   * \return none
   */
  virtual void ConfigureDevices ();

  /**
   * \brief Configure Internet stack
   * \return none
   */
  virtual void ConfigureInternet ();

  /**
   * \brief Configure mobility
   * \return none
   */
  virtual void ConfigureMobility ();

  /**
   * \brief Configure applications
   * \return none
   */
  virtual void ConfigureApplications ();

  /**
   * \brief Configure tracing
   * \return none
   */
  virtual void ConfigureTracing ();

  /**
   * \brief Configure Animation
   * \return none
   */
  virtual void ConfigureAnim ();

  /**
   * \brief Run the simulation
   * \return none
   */
  virtual void RunSimulation ();

  /**
   * \brief Process outputs
   * \return none
   */
  virtual void ProcessOutputs ();

private:
  /**
   * \brief Run the simulation
   * \return none
   */
  void Run ();

  /**
   * \brief Run the simulation
   * \param argc command line argument count
   * \param argv command line parameters
   * \return none
   */
  void CommandSetup (int argc, char **argv);

  /**
   * \brief Set up log file
   * \return none
   */
  void SetupTraceFile ();

  /**
   * \brief Set up logging
   * \return none
   */
  void SetupLogging ();

  /**
   * \brief Configure default attributes
   * \return none
   */
  void ConfigureDefaults ();

  /**
   * \brief Set up the devices
   * \return none
   */
  void SetupDevices ();

  /**
   * \brief Assign Ip Addresses for all devices
   * \return none
   */
  void AssignIpAddresses ();

  /**
   * \brief Set up the OBU mobility nodes
   * \return none
   */
  void SetupObuMobilityNodes ();

  /**
   * \brief Set up the RSU mobility nodes
   * \return none
   */
  void SetupRsuMobilityNodes ();

  /**
   * \brief Set up generation of packets
   * through the vehicular network
   * \param socket receive data
   * \return none
   */
  void ReceivePacket (Ptr<Socket> socket);

  void ReceivePacketWithAddr (std::string context, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr);

  void ReceivePacketOnSchemeFACS (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr);

  void PrintEdgeType (const EdgeType& edgeType, std::ostringstream& oss);

  void ReceivePacketOnSchemeNcb (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr);

  void ReceivePacketOnSchemeIsxd (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr);

  /**
   * \brief Set up a prescribed scenario
   * \return none
   */
  void Initialization ();

  /**
   * Course change function
   * \param context trace source context (unused)
   * \param mobility the mobility model
   */
  void CourseChange (std::string context, Ptr<const MobilityModel> mobility);

  /**
   * Course change function
   * \param os the output stream
   * \param context trace source context (unused)
   * \param mobility the mobility model
   */
  static void CourseChange (std::ostream *os, std::string context, Ptr<const MobilityModel> mobility);

  void Loop ();

  void SubmitVehRequests(Ptr<Node> obu);

  void SubmitAllVehsRequests();

  void UploadVehicleInfo (Ptr<Node> obu);

  void UploadAllVehiclesInfo ();

  void UpdateAllVehsStatus ();

  void UpdateAllFogCluster ();

  void UpdateRemainingCommuTime ();

  void DoScheduling ();

  void UpdateAllVehNeighbor ();

  void UpdateAllFogData ();

  void PrintFogCluster ();

  void FacsFunc ();

  void NcbFunc ();

  void IsxdFunc ();

  void NcbFunc1 ();

  void ResetStatusAndSend (Ptr<UdpSender> sender);

  void Decode (bool isEncoded, uint32_t broadcastId);

  void DoStats (uint32_t obuIdx, uint32_t dataIdx);

  uint32_t m_protocol; ///< protocol
  uint16_t m_dlPort;  ///< LTE down link port
  uint16_t m_ulPort;  ///< LTE up link port
  uint16_t m_v2IPort;  ///< V2I port
  uint16_t m_i2IPort;  ///< I2I port
  uint16_t m_i2VPort;  ///< I2V port
  uint16_t m_v2VPort;  ///< V2V port
  std::string m_protocolName; ///< protocol name

  double m_schedulingPeriod; ///< Scheduling period

  std::string m_time; ///< time
  std::string m_space; ///< space
  std::string m_mapSize; ///< size of map
  double m_mapWidth; ///< width of map
  double m_mapHeight; ///< height of map

  std::string m_schemeName; ///< scheme name
  std::string m_schemeParamAdjust; ///< scheme name of parameter adjust

  uint32_t m_nObuNodes; ///< number of vehicle
  NodeContainer m_obuNodes; ///< the nodes
  NetDeviceContainer m_obuDevicesTx; ///< the devices
  NetDeviceContainer m_obuDevicesRx; ///< the devices
  Ipv4InterfaceContainer m_obuInterfacesTx; ///< OBU transmit interfaces
  Ipv4InterfaceContainer m_obuInterfacesRx; ///< OBU receive interfaces
  double m_obuTxp; ///< distance
  double m_obuTxRange; ///< distance

  uint32_t m_nRsuNodes; ///< number of RSU
  NodeContainer m_rsuNodes; ///< the RSU nodes
  NetDeviceContainer m_rsu80211pDevicesTx; ///< the RSU 80211p devices
  NetDeviceContainer m_rsu80211pDevicesRx; ///< the RSU 80211p devices
  MyPointToPointStarHelper m_rsuStarHelper;
//  NetDeviceContainer m_rsuCsmaDevices; ///< the RSU CSMA devices
//  Ipv4InterfaceContainer m_rsuCsmaInterfaces; ///< RSU CSMA transmit interfaces
  Ipv4InterfaceContainer m_rsu80211pInterfacesTx; ///< RSU 80211p transmit interfaces
  Ipv4InterfaceContainer m_rsu80211pInterfacesRx; ///< RSU 80211p receive interfaces
  double m_rsuTxp; ///< distance
  double m_rsuTxRange; ///< distance

  Ptr<LteHelper> m_lteHelper;
  Ptr<PointToPointEpcHelper>  m_epcHelper;
  Ptr<Node> m_pgw;
  Ptr<Node> m_remoteHost;
  Ipv4Address m_remoteHostAddr;
  NodeContainer m_ueNodes;  ///< UE nodes
  NodeContainer m_enbNodes;  ///< eNode
  Ipv4InterfaceContainer m_ueInterface;  ///< UE ip interface

  NodeContainer m_remoteHostNodes;  ///< remote Host nodes
  double m_bsTxp; ///< distance
  double m_bsTxRange; ///< distance

  std::string m_phyMode; ///< phy mode
  bool m_traceMobility; ///< trace mobility
  std::string m_cwd; ///< current workspace directory
  std::string m_outputDir; ///< output directory
  std::string m_mobilityFile; ///< mobility file
  std::string m_enterExitStatsFile; ///< enter and exit statistics for each vehicle
  std::string m_timeNodeidLocationFile; ///< location of each vehicle at all time
  std::string m_mobLogFile; ///< mobility log file
  std::string m_trName; ///< trace file name
  std::string m_animFile; ///< animation file name
  std::string m_plotFileName; ///< plot file
  std::ofstream m_ofs;
  int m_isFileOut;
  int m_log; ///< log
  double m_totalSimTime; ///< total sim time
  int m_verbose = false;
  int m_routingTables; ///< dump routing table (at t=5 sec).  0=No, 1=Yes
  int m_asciiTrace; ///< ascii trace
  int m_pcap; ///< PCAP
  double m_timeSpent;

  PacketStats m_pktsStats; ///< request statistics
  std::vector<PacketStats> m_requestStatsEachVeh; ///< request statistics for each vehicle
  Ptr<WifiPhyStats> m_wifiPhyStats; ///< wifi phy statistics
  RoutingStats m_routingStats; ///< routing statistics
  std::map<uint32_t, PacketStats> m_requestStatsMap;

  std::string m_initReqsScheme;
  std::uint32_t m_packetSize;
  std::uint32_t m_dataSize;
  std::uint32_t m_globalDbSize;
  std::uint32_t m_minCacheSize;
  std::uint32_t m_maxCacheSize;
  std::vector<uint32_t> m_globalDB;
  std::map<uint32_t, uint32_t> m_vehId2IndexMap;
  std::vector<bool> m_vehsEnter;
  std::vector<bool> m_nodesMoving;
  std::vector<std::vector<double>> m_vehsEnterExitTime;
  std::vector<std::vector<Vector>> m_nodeidxTimeLocation; // time, nodeIdx, location
  std::vector<std::set<uint32_t>> m_vehsReqs;
  std::vector<std::set<uint32_t>> m_vehsCaches;
  std::vector<std::set<uint32_t>> m_vehsInitialReqs;
  std::vector<std::set<uint32_t>> m_vehsInitialCaches;
  std::vector<std::map<uint32_t, RequestStatus>> m_vehsReqsStatus;
  std::map<uint32_t, uint32_t> m_vehIdx2FogIdxMap;
  std::vector<std::vector<uint8_t>> m_vehNeighbors; /// neighbors of each vehicle
  std::vector<double> m_c2vRemainingTime; /// remaining communication time for c2v
  std::vector<std::vector<double>> m_v2vRemainingTime; /// remaining communication time for v2v
  std::vector<std::vector<double>> m_v2fRemainingTime; /// remaining communication time for v2f

  std::map<std::string, uint32_t> m_ipaddr2VehIdxDsrcMap;
  std::map<std::string, uint32_t> m_ipaddr2VehIdxLteMap;
  std::map<std::string, uint32_t> m_ipaddr2FogIdxDsrcMap;
  std::map<std::string, uint32_t> m_ipaddr2FogIdxCsmaMap;

  std::map<uint32_t, uint32_t> m_fogId2FogIdxMap;
  std::map<uint32_t, std::map<uint32_t, uint32_t>> m_fogIdx2FogReqInCliqueMaps;
  std::vector<std::set<uint32_t>> m_fogCluster; /// vehicles set for every fog node
  std::vector<std::set<uint32_t>> m_fogsReqs; /// fogs node request set
  std::vector<std::set<uint32_t>> m_fogsCaches; /// fogs node cache set

  std::vector<std::set<uint32_t>> m_vehsReqsInCloud;
  std::vector<std::set<uint32_t>> m_vehsCachesInCloud;
  std::vector<std::map<uint32_t, RequestStatus>> m_vehsReqsStasInCloud;
  std::vector<ns3::Vector3D> m_vehsMobInfoInCloud;

  uint32_t m_numClique;
  std::map<uint32_t, bool> m_isEncoded; // key=broadcastId
  std::vector<bool> m_isFirstSubmit;
  uint32_t m_currentBroadcastId;
  uint32_t m_dataPktId;
  std::map<uint32_t, std::vector<VertexNode>> m_broadcastId2cliqueMap; // key=broadcastId
  std::map<uint32_t, std::vector<uint32_t>> m_dataToBroadcast; // key=broadcastId
  std::map<uint32_t, std::set<uint32_t>> m_vehsToSatisfy; // key=broadcastId
  std::map<uint32_t, std::map<uint32_t, uint32_t>> m_dataToSatisfy; // key1=broadcastId, key2=vehicleId

  std::vector<std::map<uint32_t, std::set<uint32_t>>> m_extraDataForDecoding; // Extra data for decoding, where the index of the vector is the id of the vehicle, the key of map is BroadcastId.
  uint32_t m_receiveCount;

  // Scheme FACS
  std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, bool>>> isDataPktDone; // whether or not the data packet is received, key1=broadcastId, key2=vehicleId, key3=dataPktIdx
  std::map<uint32_t, std::map<uint32_t, std::map<uint32_t, uint32_t>>> m_accumulativeRcvBytes; // accumulative receive number of bytes, key1=broadcastId, key2=vehicleId, key3=dataPktIdx
  std::vector<std::vector<uint32_t>> m_statsEveryPeriod;
       /*broadcast id*/      /*vehicle id*/
  std::map<uint32_t, std::map<uint32_t, FacsResultItem>> m_FacsResultMap;// key1=broadcastId, key2=vehicleId
  std::map<uint32_t, std::map<uint32_t, bool>> m_isReceiveC2vPkt;
  ApplicationContainer m_appContainer;

  // Scheme NCB
  std::map<uint32_t, std::map<uint32_t, NcbResultItem>> m_NcbResultMap;// key1=broadcastId, key2=vehicleId
  std::list<ReqQueueItem> m_requestQueue;
  std::map<uint32_t, ReqQueueItem> m_reqQueHead;
  std::map<std::string, bool> m_requestsToMarkGloabal;
  std::map<uint32_t, std::set<std::string>> m_requestsToMarkWithBid;

  // Scheme ISXD
  std::map<uint32_t, std::map<uint32_t, IsxdResultItem>> m_IsxdResultMap;// key1=broadcastId, key2=vehicleId
};


#endif /* SCRATCH_VANET_CS_VFC_VANET_CS_VFC_H_ */
