/*
 * statistics.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statistics.h,v 1.8 2007/10/09 17:58:17 ajhseppa Exp $
 */

#ifndef __IPTV_STATISTICS_H
#define __IPTV_STATISTICS_H

#include <vdr/thread.h>

#include "statisticif.h"

// Section statistics
class cIptvSectionStatistics : public cIptvStatisticIf {
public:
  cIptvSectionStatistics();
  virtual ~cIptvSectionStatistics();
  cString GetStatistic();

protected:
  void AddStatistic(long Bytes, long Calls);

private:
  long filteredData;
  long numberOfCalls;
  cTimeMs timer;
  cMutex mutex;
};

// Device statistics
class cIptvDeviceStatistics : public cIptvStatisticIf {
public:
  cIptvDeviceStatistics();
  virtual ~cIptvDeviceStatistics();
  cString GetStatistic();

protected:
  void AddStatistic(long Bytes, u_short pid, long payload);

private:
  struct pidStruct {
    u_short pid;
    long DataAmount;
  };
  pidStruct mostActivePids[IPTV_STATS_ACTIVE_PIDS_COUNT];
  long dataBytes;
  cTimeMs timer;
  cMutex mutex;

private:
  static int SortPids(const void* data1, const void* data2);
};

// Streamer statistics
class cIptvStreamerStatistics : public cIptvStatisticIf {
public:
  cIptvStreamerStatistics();
  virtual ~cIptvStreamerStatistics();
  cString GetStatistic();

protected:
  void AddStatistic(long Bytes);

private:
  long dataBytes;
  cTimeMs timer;
  cMutex mutex;
};

// Buffer statistics
class cIptvBufferStatistics : public cIptvStatisticIf {
public:
  cIptvBufferStatistics();
  virtual ~cIptvBufferStatistics();
  cString GetStatistic();

protected:
  void AddStatistic(long used, long free);

private:
  long freeSpace;
  long usedSpace;
  cTimeMs timer;
  cMutex mutex;
};

#endif // __IPTV_STATISTICS_H

