/*
 * packet-tag-f2f.cc
 *
 *  Created on: Aug 14, 2018
 *      Author: Yang yanning <yang.ksn@gmail.com>
 */

#include "packet-tag-f2f.h"

namespace ns3 {
namespace vanet {

PacketTagF2f::PacketTagF2f (void)
{
}

void
PacketTagF2f::SetDestObusIdx (std::vector<uint32_t> destObuIdx)
{
  m_destObusIdx = destObuIdx;
}

std::vector<uint32_t>
PacketTagF2f::GetDestObusIdx (void) const
{
  return m_destObusIdx;
}


void
PacketTagF2f::SetDataIdxs (std::vector<uint32_t> dataIdxs)
{
  m_dataIdxs.assign(dataIdxs.begin(), dataIdxs.end());
}

std::vector<uint32_t>
PacketTagF2f::GetDataIdxs (void)
{
  return m_dataIdxs;
}

NS_OBJECT_ENSURE_REGISTERED (PacketTagF2f);

TypeId
PacketTagF2f::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketTagF2f")
    .SetParent<Tag> ()
    .AddConstructor<PacketTagF2f> ()
  ;
  return tid;
}
TypeId
PacketTagF2f::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
PacketTagF2f::GetSerializedSize (void) const
{
  return sizeof (uint32_t)
      + sizeof (uint32_t) + sizeof (uint32_t) * m_destObusIdx.size()
      + sizeof (uint32_t) + sizeof (uint32_t) * m_dataIdxs.size();
}
void
PacketTagF2f::Serialize (TagBuffer i) const
{
  uint32_t size1 = m_destObusIdx.size();
  i.WriteU32(size1);
  for (uint32_t obuIdx : m_destObusIdx)
    {
      i.WriteU32 (obuIdx);
    }

  uint32_t size2 = m_dataIdxs.size();
  i.WriteU32(size2);
  for (uint32_t dataIdx : m_dataIdxs)
    {
      i.WriteU32 (dataIdx);
    }
}
void
PacketTagF2f::Deserialize (TagBuffer i)
{
  uint32_t size1 = i.ReadU32 ();
  for (uint32_t j = 0; j < size1; j++)
    {
      m_destObusIdx.push_back(i.ReadU32 ());
    }

  uint32_t size2 = i.ReadU32 ();
  for (uint32_t j = 0; j < size2; j++)
    {
      m_dataIdxs.push_back(i.ReadU32 ());
    }
}
void
PacketTagF2f::Print (std::ostream &os) const
{
  os << "dataIdxs=";
  for (uint32_t dataIdx : m_dataIdxs)
    {
      os << " " << dataIdx;
    }
}

} // namespace vanet
} // namespace ns3
