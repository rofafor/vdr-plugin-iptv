/*
 * statistics.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statistics.c,v 1.7 2007/10/07 19:06:33 ajhseppa Exp $
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
  debug("cIptvSectionStatistics::GetStatistic()\n");
  uint64_t elapsed = timer.Elapsed();
  timer.Set();
  // Prevent a divide-by-zero error
  elapsed ? : elapsed = 1;
  float divider = elapsed / 1000;
  char unit[] = { ' ', 'B', '/', 's', '\0' };
  if (IptvConfig.GetStatsInKilos()) {
     divider *= KILOBYTE(1);
     unit[0] = 'k';
  }
  if (!IptvConfig.GetStatsInBytes()) {
     divider /= sizeof(unsigned short);
     unit[1] = 'b';
  }
  long tmpFilteredData = filteredData;
  long tmpNumberOfCalls = numberOfCalls;
  filteredData = numberOfCalls = 0;
  return cString::sprintf("Filtered data: %ld %s\nData packets passed: %ld\n", (long)(tmpFilteredData / divider), unit, tmpNumberOfCalls);
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
  debug("cIptvDeviceStatistics::GetStatistic()\n");
  pidStruct tmpMostActivePids[10];
  long tmpDataBytes = dataBytes;
  uint64_t elapsed = timer.Elapsed();
  timer.Set();
  // Prevent a divide-by-zero error
  elapsed ? : elapsed = 1;
  float divider = elapsed / 1000;
  char unit[] = { ' ', 'B', '/', 's', '\0' };
  if (IptvConfig.GetStatsInKilos()) {
     divider *= KILOBYTE(1);
     unit[0] = 'k';
  }
  if (!IptvConfig.GetStatsInBytes()) {
     divider /= sizeof(unsigned short);
     unit[1] = 'b';
  }
  dataBytes = 0;
  memcpy(&tmpMostActivePids, &mostActivePids, sizeof(tmpMostActivePids));
  memset(&mostActivePids, '\0', sizeof(mostActivePids));
  return cString::sprintf("Stream data bytes: %ld %s\n"
                          "  1. Active Pid: %d %ld %s\n"
                          "  2. Active Pid: %d %ld %s\n"
                          "  3. Active Pid: %d %ld %s\n"
                          "  4. Active Pid: %d %ld %s\n"
                          "  5. Active Pid: %d %ld %s\n",
                          (long)(tmpDataBytes / divider), unit,
                          tmpMostActivePids[0].pid, (long)(tmpMostActivePids[0].DataAmount / divider), unit,
                          tmpMostActivePids[1].pid, (long)(tmpMostActivePids[1].DataAmount / divider), unit,
                          tmpMostActivePids[2].pid, (long)(tmpMostActivePids[2].DataAmount / divider), unit,
                          tmpMostActivePids[3].pid, (long)(tmpMostActivePids[3].DataAmount / divider), unit,
                          tmpMostActivePids[4].pid, (long)(tmpMostActivePids[4].DataAmount / divider), unit);
}

int SortFunc(const void* data1, const void* data2)
{
  //debug("cIptvDeviceStatistics::SortFunc()\n");
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
         qsort(&mostActivePids, numberOfElements, sizeof(pidStruct), SortFunc);
         return;
         }
      }
  // Apparently our pid isn't in the array. Replace the last element with this
  // one if new payload is greater
  if (mostActivePids[numberOfElements - 1].DataAmount < payload) {
      mostActivePids[numberOfElements - 1].pid = pid;
      mostActivePids[numberOfElements - 1].DataAmount = payload;
     // Re-sort
     qsort(&mostActivePids, numberOfElements, sizeof(pidStruct), SortFunc);
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
  debug("cIptvStreamerStatistics::GetStatistic()\n");
  uint64_t elapsed = timer.Elapsed();
  timer.Set();
  // Prevent a divide-by-zero error
  elapsed ? : elapsed = 1;
  float divider = elapsed / 1000;
  char unit[] = { ' ', 'B', '/', 's', '\0' };
  if (IptvConfig.GetStatsInKilos()) {
     divider *= KILOBYTE(1);
     unit[0] = 'k';
  }
  if (!IptvConfig.GetStatsInBytes()) {
     divider /= sizeof(unsigned short);
     unit[1] = 'b';
  }
  long tmpDataBytes = (long)(dataBytes / divider);
  dataBytes = 0;
  return cString::sprintf("Stream data bytes: %ld %s\n", tmpDataBytes, unit);
}
