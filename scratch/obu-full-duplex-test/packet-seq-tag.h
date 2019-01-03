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
#ifndef PACKET_SEQ_TAG_H
#define PACKET_SEQ_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class PacketSeqTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer buf) const;
  virtual void Deserialize (TagBuffer buf);
  virtual void Print (std::ostream &os) const;
  PacketSeqTag ();
  
  /**
   *  Constructs a PacketSeqTag with the given flow id
   *
   *  \param packetSeq Id to use for the tag
   */
  PacketSeqTag (uint32_t seq);
  /**
   *  Sets the flow id for the tag
   *  \param packetSeq Id to assign to the tag
   */
  void SetPacketSeq (uint32_t packetSeq);
  /**
   *  Gets the flow id for the tag
   *  \returns current flow id for this tag
   */
  uint32_t GetPacketSeq (void) const;

  uint32_t IncPacketSeq (void);
private:
  uint32_t m_packetSeq; //!< Flow ID
};

} // namespace ns3

#endif /* PACKET_SEQ_TAG_H */
