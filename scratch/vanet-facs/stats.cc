/*
 * stats.cc
 *
 *  Created on: Aug 17, 2018
 *      Author: haha
 */

#include "stats.h"

// RequestStats
PacketStats::PacketStats ()
  : m_submittedReqs(0),
    m_satisfiedReqs(0),
    m_satisfiedReqsOneSchedule(0),
    m_broadcastPkts(0),
    m_cumulativeDelay(0),
    m_v2vTxPkts(0),
    m_v2vRxPkts(0),
    m_v2fTxPkts(0),
    m_v2fRxPkts(0)
{
}

uint32_t
PacketStats::GetSubmittedReqs ()
{
  return m_submittedReqs;
}

void
PacketStats::IncSubmittedReqs (uint32_t reqs)
{
  m_submittedReqs += reqs;
}

void
PacketStats::SetSubmittedReqs (uint32_t submittedReqs)
{
  m_submittedReqs = submittedReqs;
}

uint32_t
PacketStats::GetSatisfiedReqs ()
{
  return m_satisfiedReqs;
}

void
PacketStats::IncSatisfiedReqs ()
{
  m_satisfiedReqs++;
}

void
PacketStats::SetSatisfiedReqs (uint32_t satisfiedReqs)
{
  m_satisfiedReqs = satisfiedReqs;
}

uint32_t
PacketStats::GetSatisfiedReqsOneSchedule () const
{
  return m_satisfiedReqsOneSchedule;
}

void
PacketStats::IncSatisfiedReqsOneSchedule ()
{
  m_satisfiedReqsOneSchedule++;
}

void
PacketStats::ResetSatisfiedReqsOneSchedule ()
{
  m_satisfiedReqsOneSchedule = 0;
}

uint32_t
PacketStats::GetBroadcastPkts ()
{
  return m_broadcastPkts;
}

void
PacketStats::IncBroadcastPkts ()
{
  m_broadcastPkts++;
}

void
PacketStats::SetBroadcastPkts (uint32_t broadcastPkts)
{
  m_broadcastPkts = broadcastPkts;
}

double
PacketStats::GetCumulativeDelay ()
{
  return m_cumulativeDelay;
}

void
PacketStats::IncCumulativeDelay (double delay)
{
  m_cumulativeDelay += delay;
}

void
PacketStats::SetCumulativeDelay (double cumulativeDelay)
{
  m_cumulativeDelay = cumulativeDelay;
}

uint32_t
PacketStats::GetV2vTxPkts () const
{
  return m_v2vTxPkts;
}

void
PacketStats::IncV2vTxPkts ()
{
  m_v2vTxPkts++;
}

uint32_t
PacketStats::GetV2vRxPkts () const
{
  return m_v2vRxPkts;
}

void
PacketStats::IncV2vRxPkts ()
{
  m_v2vRxPkts++;
}

uint32_t
PacketStats::GetV2fTxPkts () const
{
  return m_v2fTxPkts;
}

void
PacketStats::IncV2fTxPkts ()
{
  m_v2fTxPkts++;
}

uint32_t
PacketStats::GetV2fRxPkts () const
{
  return m_v2fRxPkts;
}

void
PacketStats::IncV2fRxPkts ()
{
  m_v2fRxPkts++;
}

// RoutingStats
RoutingStats::RoutingStats ()
  : m_RxBytes (0),
    m_cumulativeRxBytes (0),
    m_RxPkts (0),
    m_cumulativeRxPkts (0),
    m_TxBytes (0),
    m_cumulativeTxBytes (0),
    m_TxPkts (0),
    m_cumulativeTxPkts (0)
{
}

uint32_t
RoutingStats::GetRxBytes ()
{
  return m_RxBytes;
}

uint32_t
RoutingStats::GetCumulativeRxBytes ()
{
  return m_cumulativeRxBytes;
}

uint32_t
RoutingStats::GetRxPkts ()
{
  return m_RxPkts;
}

uint32_t
RoutingStats::GetCumulativeRxPkts ()
{
  return m_cumulativeRxPkts;
}

void
RoutingStats::IncRxBytes (uint32_t rxBytes)
{
  m_RxBytes += rxBytes;
  m_cumulativeRxBytes += rxBytes;
}

void
RoutingStats::IncRxPkts ()
{
  m_RxPkts++;
  m_cumulativeRxPkts++;
}

void
RoutingStats::SetRxBytes (uint32_t rxBytes)
{
  m_RxBytes = rxBytes;
}

void
RoutingStats::SetRxPkts (uint32_t rxPkts)
{
  m_RxPkts = rxPkts;
}

uint32_t
RoutingStats::GetTxBytes ()
{
  return m_TxBytes;
}

uint32_t
RoutingStats::GetCumulativeTxBytes ()
{
  return m_cumulativeTxBytes;
}

uint32_t
RoutingStats::GetTxPkts ()
{
  return m_TxPkts;
}

uint32_t
RoutingStats::GetCumulativeTxPkts ()
{
  return m_cumulativeTxPkts;
}

void
RoutingStats::IncTxBytes (uint32_t txBytes)
{
  m_TxBytes += txBytes;
  m_cumulativeTxBytes += txBytes;
}

void
RoutingStats::IncTxPkts ()
{
  m_TxPkts++;
  m_cumulativeTxPkts++;
}

void
RoutingStats::SetTxBytes (uint32_t txBytes)
{
  m_TxBytes = txBytes;
}

void
RoutingStats::SetTxPkts (uint32_t txPkts)
{
  m_TxPkts = txPkts;
}

// WifiPhyStats
NS_LOG_COMPONENT_DEFINE ("WifiPhyStats");
NS_OBJECT_ENSURE_REGISTERED (WifiPhyStats);

TypeId
WifiPhyStats::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhyStats")
    .SetParent<Object> ()
    .AddConstructor<WifiPhyStats> ();
  return tid;
}

WifiPhyStats::WifiPhyStats ()
  : m_phyTxPkts (0),
    m_phyTxBytes (0)
{
}

WifiPhyStats::~WifiPhyStats ()
{
}

void
WifiPhyStats::PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
{
  NS_LOG_FUNCTION (this << context << packet << "PHYTX mode=" << mode );
  ++m_phyTxPkts;
  uint32_t pktSize = packet->GetSize ();
  m_phyTxBytes += pktSize;

  //NS_LOG_UNCOND ("Received PHY size=" << pktSize);
}

void
WifiPhyStats::PhyTxDrop (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_UNCOND ("PHY Tx Drop");
}

void
WifiPhyStats::PhyRxDrop (std::string context, Ptr<const Packet> packet)
{
  NS_LOG_UNCOND ("PHY Rx Drop");
}

uint32_t
WifiPhyStats::GetTxBytes ()
{
  return m_phyTxBytes;
}
