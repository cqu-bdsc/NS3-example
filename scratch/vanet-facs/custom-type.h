/*
 * graph-edge-type.h
 *
 *  Created on: Aug 16, 2018
 *      Author: Yang yanning <yang.ksn@gmail.com>
 */

#ifndef SCRATCH_VANET_CS_VFC_CUSTOM_TYPE_H_
#define SCRATCH_VANET_CS_VFC_CUSTOM_TYPE_H_

//#include <bits/stdint-uintn.h>
#include <string>
#include <sstream>
#include <ostream>

enum class EdgeType:uint8_t
{
  NOT_SET		= 0,
  CONDITION_1		= 1,
  CONDITION_2		= 2,
  CONDITION_3		= 3
};

struct ReqQueueItem
{
  uint32_t	vehIndex;
  uint32_t	reqDataIndex;
  std::string	name;

  ReqQueueItem()
  {
    this->vehIndex = 0;
    this->reqDataIndex = 0;
    this->name = "";
  }
  ReqQueueItem(const uint32_t& vehIndex, const uint32_t& _reqDataIndex)
      : vehIndex(vehIndex)
      , reqDataIndex(_reqDataIndex)
      , name("")
  {
    genName();
  }

  void genName()
  {
    std::ostringstream oss;
    oss << vehIndex << "-" << reqDataIndex;
    this->name = oss.str();
  }

  friend std::ostream & operator << (std::ostream &os, ReqQueueItem &reqItem)
    {
      os << reqItem.vehIndex << "-" << reqItem.reqDataIndex;
      return os;
    }

  bool operator == (const ReqQueueItem &reqItem) const
    {
      return (this->vehIndex == reqItem.vehIndex) && (this->reqDataIndex == reqItem.reqDataIndex);
    }
};

struct FacsResultItem
{
  uint32_t	vehIdx;
  uint32_t	dataReqed;
  int		coVehIdx;
  int		dataMerged;
  int		transMode;

  FacsResultItem()
  {
    this->vehIdx = 0;
    this->dataReqed = 0;
    this->coVehIdx = -1;
    this->dataMerged = -1;
    this->transMode = -1;
  }

  bool operator == (const FacsResultItem &item) const
    {
      return (this->vehIdx == item.vehIdx)
	  && (this->dataReqed == item.dataReqed)
	  && (this->coVehIdx == item.coVehIdx)
	  && (this->dataMerged == item.dataMerged)
	  && (this->transMode == item.transMode);
    }
};

struct NcbResultItem
{
  uint32_t	vehIdx;
  uint32_t	dataReqed1;
  int		dataReqed2;
  int		coVehIdx;
  int		fogIdx;

  NcbResultItem()
  {
    this->vehIdx = 0;
    this->dataReqed1 = 0;
    this->dataReqed2 = -1;
    this->coVehIdx = -1;
    this->fogIdx = -1;
  }

  bool operator == (const NcbResultItem &item) const
    {
      return (this->vehIdx == item.vehIdx)
	  && (this->dataReqed1 == item.dataReqed1)
	  && (this->dataReqed2 == item.dataReqed2)
	  && (this->coVehIdx == item.coVehIdx)
	  && (this->fogIdx == item.fogIdx);
    }
};

struct IsxdResultItem
{
  uint32_t	vehIdx;
  uint32_t	dataReqed;

  IsxdResultItem()
  {
    this->vehIdx = 0;
    this->dataReqed = 0;
  }

  bool operator == (const NcbResultItem &item) const
    {
      return (this->vehIdx == item.vehIdx)
	  && (this->dataReqed == item.dataReqed1);
    }
};

struct TaskItem
{
  uint32_t	srcNodeIdx;
  uint32_t	destNodeIdx;
  uint32_t	dataReqed;
  uint32_t	nextFogIdx;
  uint32_t	nextAction; // 1:f2v, 2:f2f
  std::string	name;

  TaskItem()
  {
    this->srcNodeIdx = UINT32_MAX;
    this->destNodeIdx = UINT32_MAX;
    this->dataReqed = UINT32_MAX;
    this->nextFogIdx = UINT32_MAX;
    this->nextAction = UINT32_MAX;
    this->name = "";
  }

  void genName()
  {
    std::ostringstream oss;
    oss << srcNodeIdx << "-" << destNodeIdx << "-" << dataReqed << "-" << nextAction;
    this->name = oss.str();
  }

  bool operator == (const TaskItem &item) const
    {
      return (this->srcNodeIdx == item.srcNodeIdx)
	  && (this->destNodeIdx == item.destNodeIdx)
	  && (this->dataReqed == item.dataReqed)
	  && (this->nextFogIdx == item.nextFogIdx)
	  && (this->nextAction == item.nextAction);
    }
};

#endif /* SCRATCH_VANET_CS_VFC_CUSTOM_TYPE_H_ */
