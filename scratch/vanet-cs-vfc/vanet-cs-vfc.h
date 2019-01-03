/*
 * vanet-cs-vfc.h
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
#include "ns3/gnuplot.h"
#include "graph.hpp"
#include "udp-sender.h"
#include "byte-buffer.h"
#include "packet-header.h"
#include "packet-tag-c2v.h"
#include "packet-tag-v2f.h"
#include "packet-tag-f2f.h"
#include "packet-tag-f2v.h"
#include "stats.h"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <mclmcrrt.h>
#include <mclcppclass.h>
#include <matrix.h>
#include "libMA.h"
#include "libMADecode.h"

NS_LOG_COMPONENT_DEFINE ("vanet-cs-vfc");

#define Gen_Animation 						false
#define Gen_Gnuplot_File					false

#define Lte_Enable 						false	// turn on/off LTE
#define Upload_Enable 						false
#define Cloud_Enable 						true
#define Print_Log_Header_On_Receive 				true
#define Print_Msg_Type 						true
#define Print_Edge_Type 					true
#define Print_Received_Data_Cloud 				false
#define Print_Received_Log 					false
#define Print_Vehicle_Initial_Request 				false
#define Print_Vehicle_Initial_Cache 				false
#define Print_Vehicle_Request 					false
#define Print_Vehicle_Cache 					false
#define Print_Vehicle_Final_Request 				false
#define Print_Vehicle_Final_Cache 				false
#define Print_Fog_Cluster 					false
#define Print_Total_Num_Veh_In_Fog 				false
#define Scheduling 						true
#define Print_Edge 						false
#define Search_Clique 						true
#define Print_Cliques 						true
#define Total_Time_Spent_stas 					true
#define Construct_Graph_And_Find_Clique_Time_stas 		false
#define Three_By_Three				 		false

#define Packet_Size 						1024

#define Output_Result						true
#define Console_Output_Result					false
#define Loop_Scheduling						true

#define Scheme_1						"cs-vfc"
#define Scheme_2						"ncb"
#define Scheme_3						"ma"

#define Scheme_Param_Adjust_1					"changeDbSize"
#define Scheme_Param_Adjust_2					"changeCacheSize"

#define Test_Bid						54

//#if Console_Output_Result
//
//#undef Print_Received_Log
//#define Print_Received_Log 					false
//
//#undef Print_Log_Header_On_Receive
//#define Print_Log_Header_On_Receive 				false
//
//#undef Print_Msg_Type
//#define Print_Msg_Type 						false
//
//#undef Print_Edge_Type
//#define Print_Edge_Type 					false
//
//#endif //Console_Output

/**
 * This simulation is to show the routing service of WaveNetDevice described in IEEE 09.4.
 *
 * note: although txPowerLevel is supported now, the "TxPowerLevels"
 * attribute of YansWifiPhy is 1 which means phy devices only support 1
 * levels. Thus, if users want to control txPowerLevel, they should set
 * these attributes of YansWifiPhy by themselves..
 */
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
   * \brief Check the simulation stauts
   * \return none
   */
  virtual void CheckSimulationStatus ();

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
   * \brief Set up the packet sink
   * \param addr local address
   * \param port port number
   * \param node the node to receive the packet
   * \return none
   */
  Ptr<Socket> SetupPacketSink (Ipv4Address addr, uint16_t port, Ptr<Node> node);

  /**
   * \brief Set up generation of packets
   * through the vehicular network
   * \return none
   */
  void SetupMessages ();

  /**
   * \brief Set up generation of packets
   * through the vehicular network
   * \param socket receive data
   * \return none
   */
  void ReceivePacket (Ptr<Socket> socket);

  void ReceivePacketWithAddr (std::string context, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr);

  void ReceivePacketOnSchemeCsVfc (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr);

  void PrintEdgeType (const EdgeType& edgeType, std::ostringstream& oss);

  void ReceivePacketOnSchemeNcb (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr);

  void ReceivePacketOnSchemeMA (uint32_t nodeId, Ptr<const Packet> packet, const Address & srcAddr, const Address & destAddr);

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

  void UpdateAllFogData ();

  void PrintFogCluster ();

  void ConstructGraphAndBroadcast ();

  void NcbFunc1 ();

  void MAandBroadcast ();

  void ResetStatusAndSend (Ptr<UdpSender> sender);

  void Decode (bool isEncoded, uint32_t broadcastId);

  void DoStats (uint32_t obuIdx, uint32_t dataIdx);

  uint32_t m_protocol; ///< protocol
  uint16_t m_dlPort;  ///< LTE down link port
  uint16_t m_ulPort;  ///< LTE up link port
  uint16_t m_v2IPort;  ///< V2I port
  uint16_t m_i2IPort;  ///< I2I port
  uint16_t m_i2VPort;  ///< I2V port
  std::string m_protocolName; ///< protocol name

  double m_schedulingPeriod; ///< Scheduling period

  std::string m_schemeName; ///< scheme name
  std::string m_schemeParamAdjust; ///< scheme name of parameter adjust

  uint32_t m_nObuNodes; ///< number of vehicle
  NodeContainer m_obuNodes; ///< the nodes
  NetDeviceContainer m_obuDevicesTx; ///< the devices
  Ipv4InterfaceContainer m_obuInterfaces; ///< OBU transmit interfaces
  double m_obuTxp; ///< distance

  uint32_t m_nRsuNodes; ///< number of RSU
  NodeContainer m_rsuNodes; ///< the RSU nodes
  NetDeviceContainer m_rsu80211pDevicesTx; ///< the RSU 80211p devices
  NetDeviceContainer m_rsuCsmaDevices; ///< the RSU CSMA devices
  Ipv4InterfaceContainer m_rsu80211pInterfaces; ///< RSU 80211p transmit interfaces
  Ipv4InterfaceContainer m_rsuCsmaInterfaces; ///< RSU CSMA transmit interfaces
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
  std::string m_mobilityFile; ///< mobility file
  std::string m_mobLogFile; ///< mobility log file
  std::string m_trName; ///< trace file name
  std::string m_animFile; ///< animation file name
  std::string m_plotFileName; ///< plot file
  int m_log; ///< log
  double m_totalSimTime; ///< total sim time
  std::string m_rate; ///< rate
  uint32_t m_wavePacketSize; ///< bytes
  uint32_t m_nonSafetySize; ///< bytes
  double m_waveInterval; ///< seconds
  int m_verbose = false;
  int m_routingTables; ///< dump routing table (at t=5 sec).  0=No, 1=Yes
  int m_asciiTrace; ///< ascii trace
  int m_pcap; ///< PCAP
  double m_timeSpent;

  PacketStats m_pktsStats; ///< request statistics
  Ptr<WifiPhyStats> m_wifiPhyStats; ///< wifi phy statistics
  RoutingStats m_routingStats; ///< routing statistics

  std::uint32_t m_globalDbSize;
  std::uint32_t m_minCacheSize;
  std::uint32_t m_maxCacheSize;
  std::vector<uint32_t> m_globalDB;
  std::map<uint32_t, uint32_t> m_vehId2IndexMap;
  std::vector<bool> m_vehsEnter;
  std::vector<bool> m_nodesMoving;
  std::vector<std::set<uint32_t>> m_vehsReqs;
  std::vector<std::set<uint32_t>> m_vehsCaches;
  std::vector<std::set<uint32_t>> m_vehsInitialReqs;
  std::vector<std::set<uint32_t>> m_vehsInitialCaches;
  std::vector<std::map<uint32_t, RequestStatus>> m_vehsReqsStatus;
  std::map<uint32_t, uint32_t> m_vehIdx2FogIdxMap;

  std::map<std::string, uint32_t> m_ipaddr2VehIdxDsrcMap;
  std::map<std::string, uint32_t> m_ipaddr2VehIdxLteMap;
  std::map<std::string, uint32_t> m_ipaddr2FogIdxDsrcMap;
  std::map<std::string, uint32_t> m_ipaddr2FogIdxCsmaMap;

  std::map<uint32_t, uint32_t> m_fogId2FogIdxMap;
  std::map<uint32_t, std::map<uint32_t, uint32_t>> m_fogIdx2FogReqInCliqueMaps;
  std::vector<std::set<uint32_t>> m_fogCluster; /// vehicles set for every fog node in the cloud, Updated when receive a packet from vehicle
  std::vector<std::set<uint32_t>> m_fogsReqs; /// fogs node request set in the cloud, Updated when receive a packet from vehicle
  std::vector<std::set<uint32_t>> m_fogsCaches; /// fogs node cache set in the cloud, Updated when receive a packet from vehicle

//  std::vector<std::set<uint32_t>> fogCluster; /// vehicles set for every fog node, Updated within fixed period
//  std::vector<std::set<uint32_t>> fogsReqs; /// fogs node request set, Updated within fixed period
//  std::vector<std::set<uint32_t>> fogsCaches; /// fogs node cache set, Updated within fixed period

  std::vector<std::set<uint32_t>> m_vehsReqsInCloud;
  std::vector<std::set<uint32_t>> m_vehsCachesInCloud;
  std::vector<std::map<uint32_t, RequestStatus>> m_vehsReqsStasInCloud;
  std::vector<ns3::Vector3D> m_vehsMobInfoInCloud;

  uint32_t m_numClique;
  std::map<uint32_t, bool> m_isDecoding;  // clique status
  std::vector<bool> m_isFirstSubmit;
  GraphMatrix<VertexNode> m_graph;
  std::vector<std::vector<VertexNode>> m_cliques;
  uint32_t m_currentBroadcastId;
  std::map<uint32_t, std::vector<VertexNode>> m_broadcastId2cliqueMap;

  // Extra data for decoding, where the index of the vector is the id of the vehicle, the key of map is BroadcastId.
  std::vector<std::map<uint32_t, std::set<uint32_t>>> m_extraDataForDecoding;

  uint32_t m_receiveCount;

  // Scheme NCB
  std::list<ReqQueueItem> m_requestQueue;
  std::map<uint32_t, ReqQueueItem> m_reqQueHead;
  std::map<uint32_t, std::set<uint32_t>> m_vehsToSatisfy;
  std::map<uint32_t, std::vector<uint32_t>> m_dataToBroadcast;
  std::map<std::string, bool> m_requestsToMarkGloabal;
  std::map<uint32_t, std::set<std::string>> m_requestsToMarkWithBid;

  // Scheme MA
//  std::vector<bool> vehsStatus; ///< vehicle is in the service area or not
  mwArray *mwVehsCachesMatrix; ///< cache matrix of all vehicles
};


#endif /* SCRATCH_VANET_CS_VFC_VANET_CS_VFC_H_ */
