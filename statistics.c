/*
 * statistics.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statistics.c,v 1.16 2007/10/09 17:58:17 ajhseppa Exp $
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

cString cIptvSectionStatistics::GetStatistic()
{
  //debug("cIptvSectionStatistics::GetStatistic()\n");
  cMutexLock MutexLock(&mutex);
  long bitrate = 0;
  uint64_t elapsed = timer.Elapsed(); /* in milliseconds */
  timer.Set();
  if (elapsed)
     bitrate = (long)(1000.0 * filteredData / elapsed / KILOBYTE(1));
  if (!IptvConfig.GetUseBytes())
     bitrate *= 8;
  // no trailing linefeed here!
  cString info = cString::sprintf("%4ld (%4ld k%s/s)", numberOfCalls, bitrate,
                                  IptvConfig.GetUseBytes() ? "B" : "bit");
  filteredData = numberOfCalls = 0;
  return info;
}

void cIptvSectionStatistics::AddStatistic(long Bytes, long Calls)
{
  //debug("cIptvSectionStatistics::AddStatistic(Bytes=%ld, Calls=%ld)\n", Bytes, Calls); 
  cMutexLock MutexLock(&mutex);
  filteredData += Bytes;
  numberOfCalls += Calls;
}

// --- cIptvDeviceStatistics -------------------------------------------------

// Device statistic class
cIptvDeviceStatistics::cIptvDeviceStatistics()
: dataBytes(0),
  timer(),
  mutex()
{
  debug("cIptvDeviceStatistics::cIptvDeviceStatistics()\n");
  memset(mostActivePids, '\0', sizeof(mostActivePids));
}

cIptvDeviceStatistics::~cIptvDeviceStatistics()
{
  debug("cIptvDeviceStatistics::~cIptvDeviceStatistics()\n");
}

cString cIptvDeviceStatistics::GetStatistic()
{
  //debug("cIptvDeviceStatistics::GetStatistic()\n");
  cMutexLock MutexLock(&mutex);
  long bitrate = 0L;
  uint64_t elapsed = timer.Elapsed(); /* in milliseconds */
  timer.Set();
  if (elapsed)
     bitrate = (long)(1000.0 * dataBytes / elapsed / KILOBYTE(1));
  if (!IptvConfig.GetUseBytes())
     bitrate *= 8;
  cString info = cString::sprintf("Bitrate: %ld k%s/s\n", bitrate,
                                  IptvConfig.GetUseBytes() ? "B" : "bit");
  for (unsigned int i = 0; i < IPTV_STATS_ACTIVE_PIDS_COUNT; ++i) {
      if (mostActivePids[i].pid) {
         bitrate = (long)(1000.0 * mostActivePids[i].DataAmount / elapsed / KILOBYTE(1));
         if (!IptvConfig.GetUseBytes())
            bitrate *= 8;
         info = cString::sprintf("%sPid %d: %4d (%4ld k%s/s)\n", *info, i,
                                 mostActivePids[i].pid, bitrate,
                                 IptvConfig.GetUseBytes() ? "B" : "bit");
         }
      }
  dataBytes = 0;
  memset(&mostActivePids, '\0', sizeof(mostActivePids));
  return info;
}

int cIptvDeviceStatistics::SortPids(const void* data1, const void* data2)
{
  //debug("cIptvDeviceStatistics::SortPids()\n");
  pidStruct *comp1 = (pidStruct*)data1;
  pidStruct *comp2 = (pidStruct*)data2;
  if (comp1->DataAmount > comp2->DataAmount)
     return -1;
  if (comp1->DataAmount < comp2->DataAmount)
     return 1;
  return 0;
}

void cIptvDeviceStatistics::AddStatistic(long Bytes, u_short pid, long payload)
{
  //debug("cIptvDeviceStatistics::AddStatistic(Bytes=%ld, pid=%ld, payload=%ld)\n", Bytes, pid, payload);
  cMutexLock MutexLock(&mutex);
  dataBytes += Bytes;
  const int numberOfElements = sizeof(mostActivePids) / sizeof(pidStruct);
  // If our statistic already is in the array, update it and quit
  for (int i = 0; i < numberOfElements; ++i) {
      if (mostActivePids[i].pid == pid) {
         mostActivePids[i].DataAmount += payload;
         // Now re-sort the array and quit
         qsort(&mostActivePids, numberOfElements, sizeof(pidStruct), SortPids);
         return;
         }
      }
  // Apparently our pid isn't in the array. Replace the last element with this
  // one if new payload is greater
  if (mostActivePids[numberOfElements - 1].DataAmount < payload) {
      mostActivePids[numberOfElements - 1].pid = pid;
      mostActivePids[numberOfElements - 1].DataAmount = payload;
     // Re-sort
     qsort(&mostActivePids, numberOfElements, sizeof(pidStruct), SortPids);
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

cString cIptvStreamerStatistics::GetStatistic()
{
  //debug("cIptvStreamerStatistics::GetStatistic()\n");
  cMutexLock MutexLock(&mutex);
  long bitrate = 0;
  uint64_t elapsed = timer.Elapsed(); /* in milliseconds */
  timer.Set();
  if (elapsed)
     bitrate = (long)(1000.0 * dataBytes / elapsed / KILOBYTE(1));
  if (!IptvConfig.GetUseBytes())
     bitrate *= 8;
  cString info = cString::sprintf("Streamer: %ld k%s/s\n", bitrate, IptvConfig.GetUseBytes() ? "B" : "bit");
  dataBytes = 0;
  return info;
}

void cIptvStreamerStatistics::AddStatistic(long Bytes)
{
  //debug("cIptvStreamerStatistics::AddStatistic(Bytes=%ld)\n", Bytes);
  cMutexLock MutexLock(&mutex);
  dataBytes += Bytes;
}


// Buffer statistic class
cIptvBufferStatistics::cIptvBufferStatistics()
: freeSpace(0),
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

cString cIptvBufferStatistics::GetStatistic()
{
  //debug("cIptvBufferStatistics::GetStatistic()\n");
  cMutexLock MutexLock(&mutex);
  float percentage = (float)((1-(float)freeSpace / (float)(usedSpace + freeSpace)) * 100);
  long usedKilos = (long)(usedSpace / KILOBYTE(1));
  long freeKilos = (long)(freeSpace / KILOBYTE(1));
  if (!IptvConfig.GetUseBytes()) {
     freeKilos *= 8;
     usedKilos *= 8;
     }
  cString info = cString::sprintf("%ld/%ld k%s (%2.1f%%)", usedKilos, freeKilos, IptvConfig.GetUseBytes() ? "B" : "bit", percentage);
  return info;
}

void cIptvBufferStatistics::AddStatistic(long used, long free)
{
  //debug("cIptvBufferStatistics::AddStatistic(Bytes=%ld)\n", Bytes);
  cMutexLock MutexLock(&mutex);
  freeSpace = free;
  usedSpace = used;  
}
