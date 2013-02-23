/*
 * statistics.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <limits.h>

#include "common.h"
#include "statistics.h"
#include "config.h"

// Section statistic class
cIptvSectionStatistics::cIptvSectionStatistics()
: filteredData(0),
  numberOfCalls(0),
  timer(),
  mutex()
{
  //debug("cIptvSectionStatistics::cIptvSectionStatistics()\n");
}

cIptvSectionStatistics::~cIptvSectionStatistics()
{
  //debug("cIptvSectionStatistics::~cIptvSectionStatistics()\n");
}

cString cIptvSectionStatistics::GetSectionStatistic()
{
  //debug("cIptvSectionStatistics::GetStatistic()\n");
  cMutexLock MutexLock(&mutex);
  uint64_t elapsed = timer.Elapsed(); /* in milliseconds */
  timer.Set();
  long bitrate = elapsed ? (long)(1000.0L * filteredData / KILOBYTE(1) / elapsed) : 0L;
  if (!IptvConfig.GetUseBytes())
     bitrate *= 8;
  // no trailing linefeed here!
  cString s = cString::sprintf("%4ld (%4ld k%s/s)", numberOfCalls, bitrate,
                               IptvConfig.GetUseBytes() ? "B" : "bit");
  filteredData = numberOfCalls = 0;
  return s;
}

void cIptvSectionStatistics::AddSectionStatistic(long Bytes, long Calls)
{
  //debug("cIptvSectionStatistics::AddStatistic(Bytes=%ld, Calls=%ld)\n", Bytes, Calls); 
  cMutexLock MutexLock(&mutex);
  filteredData += Bytes;
  numberOfCalls += Calls;
}

// --- cIptvPidStatistics ----------------------------------------------------

// Device statistic class
cIptvPidStatistics::cIptvPidStatistics()
: timer(),
  mutex()
{
  debug("cIptvPidStatistics::cIptvPidStatistics()\n");
  memset(mostActivePids, '\0', sizeof(mostActivePids));
}

cIptvPidStatistics::~cIptvPidStatistics()
{
  debug("cIptvPidStatistics::~cIptvPidStatistics()\n");
}

cString cIptvPidStatistics::GetPidStatistic()
{
  //debug("cIptvPidStatistics::GetStatistic()\n");
  cMutexLock MutexLock(&mutex);
  uint64_t elapsed = timer.Elapsed(); /* in milliseconds */
  timer.Set();
  cString s("Active pids:\n");
  for (unsigned int i = 0; i < IPTV_STATS_ACTIVE_PIDS_COUNT; ++i) {
      if (mostActivePids[i].pid) {
         long bitrate = elapsed ? (long)(1000.0L * mostActivePids[i].DataAmount / KILOBYTE(1) / elapsed) : 0L;
         if (!IptvConfig.GetUseBytes())
            bitrate *= 8;
         s = cString::sprintf("%sPid %d: %4d (%4ld k%s/s)\n", *s, i,
                              mostActivePids[i].pid, bitrate,
                              IptvConfig.GetUseBytes() ? "B" : "bit");
         }
      }
  memset(mostActivePids, '\0', sizeof(mostActivePids));
  return s;
}

int cIptvPidStatistics::SortPids(const void* data1, const void* data2)
{
  //debug("cIptvPidStatistics::SortPids()\n");
  const pidStruct *comp1 = reinterpret_cast<const pidStruct*>(data1);
  const pidStruct *comp2 = reinterpret_cast<const pidStruct*>(data2);
  if (comp1->DataAmount > comp2->DataAmount)
     return -1;
  if (comp1->DataAmount < comp2->DataAmount)
     return 1;
  return 0;
}

void cIptvPidStatistics::AddPidStatistic(u_short Pid, long Payload)
{
  //debug("cIptvPidStatistics::AddStatistic(pid=%ld, payload=%ld)\n", Pid, Payload);
  cMutexLock MutexLock(&mutex);
  const int numberOfElements = sizeof(mostActivePids) / sizeof(pidStruct);
  // If our statistic already is in the array, update it and quit
  for (int i = 0; i < numberOfElements; ++i) {
      if (mostActivePids[i].pid == Pid) {
         mostActivePids[i].DataAmount += Payload;
         // Now re-sort the array and quit
         qsort(mostActivePids, numberOfElements, sizeof(pidStruct), SortPids);
         return;
         }
      }
  // Apparently our pid isn't in the array. Replace the last element with this
  // one if new payload is greater
  if (mostActivePids[numberOfElements - 1].DataAmount < Payload) {
      mostActivePids[numberOfElements - 1].pid = Pid;
      mostActivePids[numberOfElements - 1].DataAmount = Payload;
     // Re-sort
     qsort(mostActivePids, numberOfElements, sizeof(pidStruct), SortPids);
     }
}

// --- cIptvStreamerStatistics -----------------------------------------------

// Streamer statistic class
cIptvStreamerStatistics::cIptvStreamerStatistics()
: dataBytes(0),
  timer(),
  mutex()
{
  debug("cIptvStreamerStatistics::cIptvStreamerStatistics()\n");
}

cIptvStreamerStatistics::~cIptvStreamerStatistics()
{
  debug("cIptvStreamerStatistics::~cIptvStreamerStatistics()\n");
}

cString cIptvStreamerStatistics::GetStreamerStatistic()
{
  //debug("cIptvStreamerStatistics::GetStatistic()\n");
  cMutexLock MutexLock(&mutex);
  uint64_t elapsed = timer.Elapsed(); /* in milliseconds */
  timer.Set();
  long bitrate = elapsed ? (long)(1000.0L * dataBytes / KILOBYTE(1) / elapsed) : 0L;
  if (!IptvConfig.GetUseBytes())
     bitrate *= 8;
  cString s = cString::sprintf("%ld k%s/s", bitrate, IptvConfig.GetUseBytes() ? "B" : "bit");
  dataBytes = 0;
  return s;
}

void cIptvStreamerStatistics::AddStreamerStatistic(long Bytes)
{
  //debug("cIptvStreamerStatistics::AddStatistic(Bytes=%ld)\n", Bytes);
  cMutexLock MutexLock(&mutex);
  dataBytes += Bytes;
}


// Buffer statistic class
cIptvBufferStatistics::cIptvBufferStatistics()
: dataBytes(0),
  freeSpace(0),
  usedSpace(0),
  timer(),
  mutex()
{
  debug("cIptvBufferStatistics::cIptvBufferStatistics()\n");
}

cIptvBufferStatistics::~cIptvBufferStatistics()
{
  debug("cIptvBufferStatistics::~cIptvBufferStatistics()\n");
}

cString cIptvBufferStatistics::GetBufferStatistic()
{
  //debug("cIptvBufferStatistics::GetStatistic()\n");
  cMutexLock MutexLock(&mutex);
  uint64_t elapsed = timer.Elapsed(); /* in milliseconds */
  timer.Set();
  long bitrate = elapsed ? (long)(1000.0L * dataBytes / KILOBYTE(1) / elapsed) : 0L;
  long totalSpace = MEGABYTE(IptvConfig.GetTsBufferSize());
  float percentage = (float)((float)usedSpace / (float)totalSpace * 100.0);
  long totalKilos = totalSpace / KILOBYTE(1);
  long usedKilos = usedSpace / KILOBYTE(1);
  if (!IptvConfig.GetUseBytes()) {
     bitrate *= 8;
     totalKilos *= 8;
     usedKilos *= 8;
     }
  cString s = cString::sprintf("Buffer bitrate: %ld k%s/s\nBuffer usage: %ld/%ld k%s (%2.1f%%)\n", bitrate,
                               IptvConfig.GetUseBytes() ? "B" : "bit", usedKilos, totalKilos,
                               IptvConfig.GetUseBytes() ? "B" : "bit", percentage);
  dataBytes = 0;
  usedSpace = 0;
  return s;
}

void cIptvBufferStatistics::AddBufferStatistic(long Bytes, long Used)
{
  //debug("cIptvBufferStatistics::AddStatistic(Bytes=%ld, Used=%ld)\n", Bytes, Used);
  cMutexLock MutexLock(&mutex);
  dataBytes += Bytes;
  if (Used > usedSpace)
     usedSpace = Used;
}
