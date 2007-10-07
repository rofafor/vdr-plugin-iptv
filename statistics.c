/*
 * statistics.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statistics.c,v 1.5 2007/10/07 08:46:34 ajhseppa Exp $
 */

#include "common.h"
#include "statistics.h"

// Section statistic class
cIptvSectionStatistics::cIptvSectionStatistics()
: filteredData(0),
  numberOfCalls(0)
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
  long tmpFilteredData = filteredData;
  long tmpNumberOfCalls = numberOfCalls;
  filteredData = numberOfCalls = 0;
  return cString::sprintf("Filtered data: %ld\nData packets passed: %ld\n", tmpFilteredData, tmpNumberOfCalls);
}

// --- cIptvDeviceStatistics -------------------------------------------------

// Device statistic class
cIptvDeviceStatistics::cIptvDeviceStatistics()
: dataBytes(0)
{
  debug("cIptvDeviceStatistics::cIptvDeviceStatistics()\n");
  memset(mostActivePids, '\0', sizeof(mostActivePids));
}

cIptvDeviceStatistics::~cIptvDeviceStatistics()
{
  debug("cIptvDeviceStatistics::~cIptvDeviceStatistics()\n");
}

cString cIptvDeviceStatistics::GetStatistic(void)
{
  debug("cIptvDeviceStatistics::GetStatistic()\n");
  pidStruct tmpMostActivePids[10];
  long tmpDataBytes = dataBytes;
  dataBytes = 0;
  memcpy(&tmpMostActivePids, &mostActivePids, sizeof(tmpMostActivePids));
  memset(&mostActivePids, '\0', sizeof(mostActivePids));
  return cString::sprintf("Stream data bytes: %ld\n1. Active Pid: %d %ld\n2. Active Pid: %d %ld\n3. Active Pid: %d %ld\n",
                          tmpDataBytes,
                          tmpMostActivePids[0].pid, tmpMostActivePids[0].DataAmount,
                          tmpMostActivePids[1].pid, tmpMostActivePids[1].DataAmount,
                          tmpMostActivePids[2].pid, tmpMostActivePids[2].DataAmount);
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
: dataBytes(0)
{
  debug("cIptvStreamerStatistics::cIptvStreamerStatistics()\n");
}

cIptvStreamerStatistics::~cIptvStreamerStatistics()
{
  debug("cIptvStreamerStatistics::~cIptvStreamerStatistics()\n");
}

cString cIptvStreamerStatistics::GetStatistic(void)
{
  debug("cIptvStreamerStatistics::GetStatistic()\n");
  long tmpDataBytes = dataBytes;
  dataBytes = 0;
  return cString::sprintf("Stream data bytes: %ld\n", tmpDataBytes);
}
