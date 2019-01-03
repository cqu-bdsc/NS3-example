/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ns3/log.h"
#include "packet-seq-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PacketSeqTag");

NS_OBJECT_ENSURE_REGISTERED (PacketSeqTag);

TypeId 
PacketSeqTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketSeqTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<PacketSeqTag> ()
  ;
  return tid;
}
TypeId 
PacketSeqTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
PacketSeqTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 4;
}
void 
PacketSeqTag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
  buf.WriteU32 (m_packetSeq);
}
void 
PacketSeqTag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
  m_packetSeq = buf.ReadU32 ();
}
void 
PacketSeqTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "PacketSeq=" << m_packetSeq;
}
PacketSeqTag::PacketSeqTag ()
  : Tag (),
    m_packetSeq (0)
{
  NS_LOG_FUNCTION (this);
}

PacketSeqTag::PacketSeqTag (uint32_t seq)
  : Tag (),
    m_packetSeq (seq)
{
  NS_LOG_FUNCTION (this << seq);
}

void
PacketSeqTag::SetPacketSeq (uint32_t id)
{
  NS_LOG_FUNCTION (this << id);
  m_packetSeq = id;
}
uint32_t
PacketSeqTag::GetPacketSeq (void) const
{
  NS_LOG_FUNCTION (this);
  return m_packetSeq;
}

uint32_t 
PacketSeqTag::IncPacketSeq (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_packetSeq++;
}

} // namespace ns3

