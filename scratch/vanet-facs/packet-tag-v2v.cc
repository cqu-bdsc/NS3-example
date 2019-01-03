/*
 * packet-tag-v2v.cc
 *
 *  Created on: Otc. 29, 2018
 *      Author: Yang yanning <yang.ksn@gmail.com>
 */

#include "packet-tag-v2v.h"

namespace ns3 {
namespace vanet {

PacketTagV2v::PacketTagV2v (void)
{
}

void
PacketTagV2v::SetSrcNodeId(uint32_t srcNodeId)
{
  m_srcNodeId = srcNodeId;
}

uint32_t
PacketTagV2v::GetSrcNodeId()
{
  return m_srcNodeId;
}

void
PacketTagV2v::SetDataIdxs (std::vector<uint32_t> dataIdxs)
{
  m_dataIdxs.assign(dataIdxs.begin(), dataIdxs.end());
}

std::vector<uint32_t>
PacketTagV2v::GetDataIdxs (void) const
{
  return m_dataIdxs;
}

NS_OBJECT_ENSURE_REGISTERED (PacketTagV2v);

TypeId
PacketTagV2v::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketTagV2v")
    .SetParent<Tag> ()
    .AddConstructor<PacketTagV2v> ()
  ;
  return tid;
}
TypeId
PacketTagV2v::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
PacketTagV2v::GetSerializedSize (void) const
{
  return sizeof (uint32_t)
      + sizeof (uint32_t) + sizeof (uint32_t) * m_dataIdxs.size();
}
void
PacketTagV2v::Serialize (TagBuffer i) const
{
  i.WriteU32(m_srcNodeId);

  uint32_t size = m_dataIdxs.size();
  i.WriteU32(size);
  for (uint32_t dataIdx : m_dataIdxs)
    {
      i.WriteU32 (dataIdx);
    }
}
void
PacketTagV2v::Deserialize (TagBuffer i)
{
  m_srcNodeId = i.ReadU32();

  uint32_t size = i.ReadU32 ();
  for (uint32_t j = 0; j < size; j++)
    {
      m_dataIdxs.push_back(i.ReadU32 ());
    }
}
void
PacketTagV2v::Print (std::ostream &os) const
{
  os << "srcNodeId=" << m_srcNodeId;
  os << ", dataIdxs=";
  for (uint32_t dataIdx : m_dataIdxs)
    {
      os << " " << dataIdx;
    }
}

} // namespace vanet
} // namespace ns3
