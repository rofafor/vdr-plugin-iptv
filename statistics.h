/*
 * statistics.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statistics.h,v 1.3 2007/10/07 10:13:45 ajhseppa Exp $
 */

#ifndef __IPTV_STATISTICS_H
#define __IPTV_STATISTICS_H

#include "statisticif.h"

struct pidStruct {
  u_short pid;
  long DataAmount;
};

// Section statistics
class cIptvSectionStatistics : public cIptvStatisticIf {
protected:
  long filteredData;
  long numberOfCalls;

public:
  cIptvSectionStatistics();
  virtual ~cIptvSectionStatistics();

  cString GetStatistic(uint64_t elapsed, const char* unit, int unitdivider);
};

// Device statistics
class cIptvDeviceStatistics : public cIptvStatisticIf {
protected:
  long dataBytes;

public:
  cIptvDeviceStatistics();
  virtual ~cIptvDeviceStatistics();

  cString GetStatistic(uint64_t elapsed, const char* unit, int unitdivider);

protected:
  void UpdateActivePids(u_short pid, long payload);

private:
  pidStruct mostActivePids[10];
};

// Streamer statistics
class cIptvStreamerStatistics : public cIptvStatisticIf {
protected:
  long dataBytes;

public:
  cIptvStreamerStatistics();
  virtual ~cIptvStreamerStatistics();

  cString GetStatistic(uint64_t elapsed, const char* unit, int unitdivider);
};

#endif // __IPTV_STATISTICS_H

