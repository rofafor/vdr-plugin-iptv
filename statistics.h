/*
 * statistics.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statistics.h,v 1.1 2007/10/05 19:00:44 ajhseppa Exp $
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

  char* GetStatistic();
};

// Device statistics
class cIptvDeviceStatistics : public cIptvStatisticIf {
protected:
  long dataBytes;

public:
  cIptvDeviceStatistics();
  virtual ~cIptvDeviceStatistics();

  char* GetStatistic();

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

  char* GetStatistic();
};

#endif // __IPTV_STATISTICS_H

