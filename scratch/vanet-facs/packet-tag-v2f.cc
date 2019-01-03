/*
 * packet-tag-v2f.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: Yang yanning <yang.ksn@gmail.com>
 */

#include "packet-tag-v2f.h"

namespace ns3 {
namespace vanet {

PacketTagV2f::PacketTagV2f (void)
{
}

void
PacketTagV2f::SetNextActionType (PacketTagV2f::NextActionType type)
{
  switch (type)
    {
    case NextActionType::NOT_SET:
      m_nextActionType = 0;
      break;
    case NextActionType::F2F:
      m_nextActionType = 1;
      break;
    case NextActionType::F2V:
      m_nextActionType = 2;
      break;
    default:
      NS_FATAL_ERROR ("Unknown NextAction-Type: " << static_cast<uint8_t> (type));
      break;
    }
}
PacketTagV2f::NextActionType
PacketTagV2f::GetNextActionType (void) const
{
  NextActionType ret;
  switch (m_nextActionType)
    {
    case 0:
      ret = NextActionType::NOT_SET;
      break;
    case 1:
      ret = NextActionType::F2F;
      break;
    case 2:
      ret = NextActionType::F2V;
      break;
    default:
      NS_FATAL_ERROR ("Unknown NextAction-Type: " << m_nextActionType);
      break;
    }
  return ret;
}

void
PacketTagV2f::SetNextObusIdx (std::vector<uint32_t> nextObusIdx)
{
  m_nextObusIdx = nextObusIdx;
}

std::vector<uint32_t>
PacketTagV2f::GetNextObusIdx (void) const
{
  return m_nextObusIdx;
}

void
PacketTagV2f::SetNextRsusIdx (std::vector<uint32_t> nextRsusIdx)
{
  m_nextRsusIdx.assign(nextRsusIdx.begin(), nextRsusIdx.end());
}

std::vector<uint32_t>
PacketTagV2f::GetNextRsusIdx (void) const
{
  return m_nextRsusIdx;
}

void
PacketTagV2f::SetDestObusIdx (std::map<uint32_t, std::vector<uint32_t>> destObusIdx)
{
  m_destObusIdx = destObusIdx;
}

std::map<uint32_t, std::vector<uint32_t>>
PacketTagV2f::GetDestObusIdx (void) const
{
  return m_destObusIdx;
}

void
PacketTagV2f::SetDataIdxs (std::vector<uint32_t> dataIdxs)
{
  m_dataIdxs.assign(dataIdxs.begin(), dataIdxs.end());
}

std::vector<uint32_t>
PacketTagV2f::GetDataIdxs (void) const
{
  return m_dataIdxs;
}

NS_OBJECT_ENSURE_REGISTERED (PacketTagV2f);

TypeId
PacketTagV2f::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketTagV2f")
    .SetParent<Tag> ()
    .AddConstructor<PacketTagV2f> ()
  ;
  return tid;
}
TypeId
PacketTagV2f::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
PacketTagV2f::GetSerializedSize (void) const
{
  uint32_t destObusIdxSize = 0;
  std::map<uint32_t, std::vector<uint32_t>>::const_iterator mIt = m_destObusIdx.begin();
  for (; mIt != m_destObusIdx.end(); ++mIt)
    {
      destObusIdxSize += (sizeof (uint32_t) + sizeof (uint32_t) + sizeof (uint32_t) * mIt->second.size());
    }

  return sizeof (uint8_t)
      + sizeof (uint32_t) + sizeof (uint32_t) * m_nextObusIdx.size()
      + sizeof (uint32_t) + sizeof (uint32_t) * m_nextRsusIdx.size()
      + sizeof (uint32_t) + destObusIdxSize
      + sizeof (uint32_t) + sizeof (uint32_t) * m_dataIdxs.size();
}
void
PacketTagV2f::Serialize (TagBuffer i) const
{
  i.WriteU8(m_nextActionType);

  uint32_t size00 = m_nextObusIdx.size();
  i.WriteU32(size00);
  for (uint32_t obuIdx : m_nextObusIdx)
    {
      i.WriteU32 (obuIdx);
    }

  uint32_t size0 = m_nextRsusIdx.size();
  i.WriteU32(size0);
  for (uint32_t rsuIdx : m_nextRsusIdx)
    {
      i.WriteU32 (rsuIdx);
    }

  uint32_t size1 = m_destObusIdx.size();
  i.WriteU32(size1);
  std::map<uint32_t, std::vector<uint32_t>>::const_iterator mIt = m_destObusIdx.begin();
  for (; mIt != m_destObusIdx.end(); ++mIt)
    {
      i.WriteU32 (mIt->first);
      i.WriteU32 (mIt->second.size());
      for (uint32_t obuIdx : mIt->second)
	{
	  i.WriteU32 (obuIdx);
	}
    }

  uint32_t size2 = m_dataIdxs.size();
  i.WriteU32(size2);
  for (uint32_t dataIdx : m_dataIdxs)
    {
      i.WriteU32 (dataIdx);
    }
}
void
PacketTagV2f::Deserialize (TagBuffer i)
{
  m_nextActionType = i.ReadU8();

  uint32_t size00 = i.ReadU32 ();
  for (uint32_t j = 0; j < size00; j++)
    {
      m_nextObusIdx.push_back(i.ReadU32 ());
    }

  uint32_t size0 = i.ReadU32 ();
  for (uint32_t j = 0; j < size0; j++)
    {
      m_nextRsusIdx.push_back(i.ReadU32 ());
    }

  uint32_t size1 = i.ReadU32 ();
  for (uint32_t j = 0; j < size1; j++)
    {
      uint32_t key = i.ReadU32 ();
      std::vector<uint32_t> destObusIdx;
      uint32_t size11 = i.ReadU32 ();
      for (uint32_t k = 0; k < size11; k++)
	{
	  destObusIdx.push_back(i.ReadU32());
	}
      m_destObusIdx[key] = destObusIdx;
    }

  uint32_t size2 = i.ReadU32 ();
  for (uint32_t j = 0; j < size2; j++)
    {
      m_dataIdxs.push_back(i.ReadU32 ());
    }
}
void
PacketTagV2f::Print (std::ostream &os) const
{
  os << "nextActionType=" << m_nextActionType;
  os << ", rsuWaitingServedIdxs=";
  for (uint32_t rsuIdx : m_nextRsusIdx)
    {
      os << " " << rsuIdx;
    }
  os << ", dataIdxs=";
  for (uint32_t dataIdx : m_dataIdxs)
    {
      os << " " << dataIdx;
    }
}

} // namespace vanet
} // namespace ns3
