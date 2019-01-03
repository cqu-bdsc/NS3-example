/*
 * packet-tag-f2v.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: Yang yanning <yang.ksn@gmail.com>
 */

#include "packet-tag-f2v.h"

namespace ns3 {
namespace vanet {

PacketTagF2v::PacketTagF2v (void)
{
}

void
PacketTagF2v::SetSrcNodeId(uint32_t srcNodeId)
{
  m_srcNodeId = srcNodeId;
}

uint32_t
PacketTagF2v::GetSrcNodeId()
{
  return m_srcNodeId;
}

void
PacketTagF2v::SetDataIdxs (std::vector<uint32_t> dataIdxs)
{
  m_dataIdxs.assign(dataIdxs.begin(), dataIdxs.end());
}

std::vector<uint32_t>
PacketTagF2v::GetDataIdxs (void)
{
  return m_dataIdxs;
}

NS_OBJECT_ENSURE_REGISTERED (PacketTagF2v);

TypeId
PacketTagF2v::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketTagF2v")
    .SetParent<Tag> ()
    .AddConstructor<PacketTagF2v> ()
  ;
  return tid;
}
TypeId
PacketTagF2v::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
PacketTagF2v::GetSerializedSize (void) const
{
  return sizeof (uint32_t)
      + sizeof (uint32_t) + sizeof (uint32_t) * m_dataIdxs.size();
}
void
PacketTagF2v::Serialize (TagBuffer i) const
{
  i.WriteU32(m_srcNodeId);

  uint32_t size2 = m_dataIdxs.size();
  i.WriteU32(size2);
  for (uint32_t dataIdx : m_dataIdxs)
    {
      i.WriteU32 (dataIdx);
    }
}
void
PacketTagF2v::Deserialize (TagBuffer i)
{
  m_srcNodeId = i.ReadU32();

  uint32_t size2 = i.ReadU32 ();
  for (uint32_t j = 0; j < size2; j++)
    {
      m_dataIdxs.push_back(i.ReadU32 ());
    }
}
void
PacketTagF2v::Print (std::ostream &os) const
{
  os << "dataIdxs=";
  for (uint32_t dataIdx : m_dataIdxs)
    {
      os << " " << dataIdx;
    }
}

} // namespace vanet
} // namespace ns3
