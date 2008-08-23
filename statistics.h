/*
 * statistics.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_STATISTICS_H
#define __IPTV_STATISTICS_H

#include <vdr/thread.h>

// Section statistics
class cIptvSectionStatistics {
public:
  cIptvSectionStatistics();
  virtual ~cIptvSectionStatistics();
  cString GetSectionStatistic();

protected:
  void AddSectionStatistic(long Bytes, long Calls);

private:
  long filteredData;
  long numberOfCalls;
  cTimeMs timer;
  cMutex mutex;
};

// Pid statistics
class cIptvPidStatistics {
public:
  cIptvPidStatistics();
  virtual ~cIptvPidStatistics();
  cString GetPidStatistic();

protected:
  void AddPidStatistic(u_short Pid, long Payload);

private:
  struct pidStruct {
    u_short pid;
    long DataAmount;
  };
  pidStruct mostActivePids[IPTV_STATS_ACTIVE_PIDS_COUNT];
  cTimeMs timer;
  cMutex mutex;

private:
  static int SortPids(const void* data1, const void* data2);
};

// Streamer statistics
class cIptvStreamerStatistics {
public:
  cIptvStreamerStatistics();
  virtual ~cIptvStreamerStatistics();
  cString GetStreamerStatistic();

protected:
  void AddStreamerStatistic(long Bytes);

private:
  long dataBytes;
  cTimeMs timer;
  cMutex mutex;
};

// Buffer statistics
class cIptvBufferStatistics {
public:
  cIptvBufferStatistics();
  virtual ~cIptvBufferStatistics();
  cString GetBufferStatistic();

protected:
  void AddBufferStatistic(long Bytes, long Used);

private:
  long dataBytes;
  long freeSpace;
  long usedSpace;
  cTimeMs timer;
  cMutex mutex;
};

#endif // __IPTV_STATISTICS_H
