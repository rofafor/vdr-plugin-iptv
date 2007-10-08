/*
 * statistics.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statistics.c,v 1.12 2007/10/08 12:30:53 rahrenbe Exp $
 */

#include <limits.h>

#include "common.h"
#include "statistics.h"
#include "config.h"

// Section statistic class
cIptvSectionStatistics::cIptvSectionStatistics()
: filteredData(0),
  numberOfCalls(0),
  timer()
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
  uint64_t elapsed = timer.Elapsed();
  timer.Set();
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
  cString info = cString::sprintf("%4ld (%4ld %s)", numberOfCalls, divider ?
                                  (long)(filteredData / divider) : 0L, unit);
  filteredData = numberOfCalls = 0;
  return info;
}

// --- cIptvDeviceStatistics -------------------------------------------------

// Device statistic class
cIptvDeviceStatistics::cIptvDeviceStatistics()
: dataBytes(0),
  timer()
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
  long tmpDataBytes = dataBytes;
  uint64_t elapsed = timer.Elapsed();
  timer.Set();
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
      if (mostActivePids[i].pid)
         info = cString::sprintf("%sPid %d: %4d (%4ld %s)%c", *info, i,
                                 mostActivePids[i].pid,
                                 (long)(mostActivePids[i].DataAmount / divider),
                                 unit, ((i + 1) % 2) ? '\t' : '\n');
      }
  if (!endswith(*info, "\n"))
     info = cString::sprintf("%s%c", *info, '\n');
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

void cIptvDeviceStatistics::UpdateActivePids(u_short pid, long payload)
{
  //debug("cIptvDeviceStatistics::UpdateActivePids()\n");
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
  timer()
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
  uint64_t elapsed = timer.Elapsed();
  timer.Set();
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
  long tmpDataBytes = divider ? (long)(dataBytes / divider) : 0L;
  dataBytes = 0;
  return cString::sprintf("Streamer: %ld %s", tmpDataBytes, unit);
}
