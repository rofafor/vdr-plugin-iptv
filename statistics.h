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
  void AddSectionStatistic(long bytesP, long callsP);

private:
  long filteredDataM;
  long numberOfCallsM;
  cTimeMs timerM;
  cMutex mutexM;
};

// Pid statistics
class cIptvPidStatistics {
public:
  cIptvPidStatistics();
  virtual ~cIptvPidStatistics();
  cString GetPidStatistic();

protected:
  void AddPidStatistic(int pidP, long payloadP);

private:
  struct pidStruct {
    int  pid;
    long dataAmount;
  };
  pidStruct mostActivePidsM[IPTV_STATS_ACTIVE_PIDS_COUNT];
  cTimeMs timerM;
  cMutex mutexM;

private:
  static int SortPids(const void* data1P, const void* data2P);
};

// Streamer statistics
class cIptvStreamerStatistics {
public:
  cIptvStreamerStatistics();
  virtual ~cIptvStreamerStatistics();
  cString GetStreamerStatistic();

protected:
  void AddStreamerStatistic(long bytesP);

private:
  long dataBytesM;
  cTimeMs timerM;
  cMutex mutexM;
};

// Buffer statistics
class cIptvBufferStatistics {
public:
  cIptvBufferStatistics();
  virtual ~cIptvBufferStatistics();
  cString GetBufferStatistic();

protected:
  void AddBufferStatistic(long bytesP, long usedP);

private:
  long dataBytesM;
  long freeSpaceM;
  long usedSpaceM;
  cTimeMs timerM;
  cMutex mutexM;
};

#endif // __IPTV_STATISTICS_H
