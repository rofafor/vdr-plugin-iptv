/*
 * statistics.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statistics.c,v 1.14 2007/10/08 18:31:44 ajhseppa Exp $
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
  mutex.Lock();
  long tmpNumberOfCalls = numberOfCalls;
  long tmpFilteredData = filteredData;
  filteredData = numberOfCalls = 0;
  uint64_t elapsed = timer.Elapsed();
  timer.Set();
  mutex.Unlock();
  float divider = elapsed / 1000;
  char unit[] = { ' ', 'B', '/', 's', '\0' };
  if (IptvConfig.IsStatsUnitInKilos()) {
     divider *= KILOBYTE(1);
     unit[0] = 'k';
     }
  if (!IptvConfig.IsStatsUnitInBytes()) {
     divider /= sizeof(unsigned short) * 8;
     unit[1] = 'b';
     }
  cString info = cString::sprintf("%4ld (%4ld %s)", tmpNumberOfCalls, divider ?
                                  (long)(tmpFilteredData / divider) : 0L, unit);
  return info;
}

void cIptvSectionStatistics::AddStatistic(long Bytes, long Calls)
{
  //debug("cIptvSectionStatistics::AddStatistic(Bytes=%ld, Calls=%ld)\n", Bytes, Calls); 
  mutex.Lock();
  filteredData += Bytes;
  numberOfCalls += Calls;
  mutex.Unlock();
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
  mutex.Lock();
  long tmpDataBytes = dataBytes;
  dataBytes = 0;
  pidStruct tmpMostActivePids[IPTV_STATS_ACTIVE_PIDS_COUNT];
  memcpy(&tmpMostActivePids, &mostActivePids, sizeof(tmpMostActivePids));
  memset(&mostActivePids, '\0', sizeof(mostActivePids));
  uint64_t elapsed = timer.Elapsed();
  timer.Set();
  mutex.Unlock();
  float divider = elapsed / 1000;
  char unit[] = { ' ', 'B', '/', 's', '\0' };
  if (IptvConfig.IsStatsUnitInKilos()) {
     divider *= KILOBYTE(1);
     unit[0] = 'k';
     }
  if (!IptvConfig.IsStatsUnitInBytes()) {
     divider /= sizeof(unsigned short) * 8;
     unit[1] = 'b';
     }
  cString info = cString::sprintf("Bitrate: %ld %s\n", divider ?
                                  (long)(tmpDataBytes / divider) : 0L, unit);
  for (unsigned int i = 0; i < IPTV_STATS_ACTIVE_PIDS_COUNT; ++i) {
      if (tmpMostActivePids[i].pid)
         info = cString::sprintf("%sPid %d: %4d (%4ld %s)%c", *info, i,
                                 tmpMostActivePids[i].pid,
                                 (long)(tmpMostActivePids[i].DataAmount / divider),
                                 unit, ((i + 1) % 2) ? '\t' : '\n');
      }
  if (!endswith(*info, "\n"))
     info = cString::sprintf("%s%c", *info, '\n');
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
  mutex.Lock();
  dataBytes += Bytes;
  const int numberOfElements = sizeof(mostActivePids) / sizeof(pidStruct);
  // If our statistic already is in the array, update it and quit
  for (int i = 0; i < numberOfElements; ++i) {
      if (mostActivePids[i].pid == pid) {
         mostActivePids[i].DataAmount += payload;
         // Now re-sort the array and quit
         qsort(&mostActivePids, numberOfElements, sizeof(pidStruct), SortPids);
         mutex.Unlock();
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
  mutex.Unlock();
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
  mutex.Lock();
  long tmpDataBytes = dataBytes;
  dataBytes = 0;
  uint64_t elapsed = timer.Elapsed();
  timer.Set();
  mutex.Unlock();
  float divider = elapsed / 1000;
  char unit[] = { ' ', 'B', '/', 's', '\0' };
  if (IptvConfig.IsStatsUnitInKilos()) {
     divider *= KILOBYTE(1);
     unit[0] = 'k';
     }
  if (!IptvConfig.IsStatsUnitInBytes()) {
     divider /= sizeof(unsigned short) * 8;
     unit[1] = 'b';
     }
  return cString::sprintf("Streamer: %ld %s", divider ?
                          (long)(tmpDataBytes / divider) : 0L, unit);
}

void cIptvStreamerStatistics::AddStatistic(long Bytes)
{
  //debug("cIptvStreamerStatistics::AddStatistic(Bytes=%ld)\n", Bytes);
  mutex.Lock();
  dataBytes += Bytes;
  mutex.Unlock();
}
