/*
 * packet-tag-f2v.h
 *
 *  Created on: Aug 14, 2018
 *      Author: Yang yanning <yang.ksn@gmail.com>
 */

#ifndef SCRATCH_VANET_CS_VFC_PACKET_TAG_F2V_H_
#define SCRATCH_VANET_CS_VFC_PACKET_TAG_F2V_H_

#include <vector>

#include "custom-type.h"
#include "ns3/tag.h"

namespace ns3 {
namespace vanet {

class PacketTagF2v : public Tag
{
public:

  PacketTagF2v (void);

  void SetSrcNodeId(uint32_t srcNodeId);
  uint32_t GetSrcNodeId(void);

  void SetDataIdxs (std::vector<uint32_t> dataIdxs);
  std::vector<uint32_t> GetDataIdxs (void);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

private:
  uint32_t m_srcNodeId;
  std::vector<uint32_t> m_dataIdxs;
};

} // namespace vanet
} // namespace ns3

#endif /* SCRATCH_VANET_CS_VFC_PACKET_TAG_F2V_H_ */
