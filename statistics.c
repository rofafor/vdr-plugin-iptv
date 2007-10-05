/*
 * statistics.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statistics.c,v 1.3 2007/10/05 22:30:14 ajhseppa Exp $
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

char* cIptvSectionStatistics::GetStatistic()
{
  debug("cIptvSectionStatistics::GetStatistic()\n");
  char* retval;
  asprintf(&retval, "Filtered data: %ld Data packets passed: %ld",
	   filteredData, numberOfCalls);

  filteredData = numberOfCalls = 0;

  return retval;
}

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

char* cIptvDeviceStatistics::GetStatistic()
{
  debug("cIptvDeviceStatistics::GetStatistic()\n");
  char* retval;
  asprintf(&retval, "Stream data bytes: %ld, Active Pids: 1: Pid %d %ld 2: Pid %d %ld 3: %d %ld", dataBytes, mostActivePids[0].pid,
           mostActivePids[0].DataAmount, mostActivePids[1].pid,
           mostActivePids[1].DataAmount, mostActivePids[2].pid,
           mostActivePids[2].DataAmount);

  dataBytes = 0;
  memset(&mostActivePids, '\0', sizeof(mostActivePids));

  return retval;
}

int SortFunc(const void* data1, const void* data2)
{
  //debug("cIptvDeviceStatistics::SortFunc()\n");

  pidStruct *comp1 = (pidStruct*)data1;
  pidStruct *comp2 = (pidStruct*)data2;

  if (comp1->DataAmount > comp2->DataAmount)
     return 1;

  if (comp1->DataAmount < comp2->DataAmount)
     return -1;

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

char* cIptvStreamerStatistics::GetStatistic()
{
  debug("cIptvStreamerStatistics::GetStatistic()\n");
  char* retval;
  asprintf(&retval, "Stream data bytes: %ld", dataBytes);

  dataBytes = 0;

  return retval;
}
