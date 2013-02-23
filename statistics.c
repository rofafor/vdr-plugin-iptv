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

// Section statistics class
cIptvSectionStatistics::cIptvSectionStatistics()
: filteredDataM(0),
  numberOfCallsM(0),
  timerM(),
  mutexM()
{
  //debug("cIptvSectionStatistics::%s()", __FUNCTION__);
}

cIptvSectionStatistics::~cIptvSectionStatistics()
{
  //debug("cIptvSectionStatistics::%s()", __FUNCTION__);
}

cString cIptvSectionStatistics::GetSectionStatistic()
{
  //debug("cIptvSectionStatistics::%s()", __FUNCTION__);
  cMutexLock MutexLock(&mutexM);
  uint64_t elapsed = timerM.Elapsed(); /* in milliseconds */
  timerM.Set();
  long bitrate = elapsed ? (long)(1000.0L * filteredDataM / KILOBYTE(1) / elapsed) : 0L;
  if (!IptvConfig.GetUseBytes())
     bitrate *= 8;
  // no trailing linefeed here!
  cString s = cString::sprintf("%4ld (%4ld k%s/s)", numberOfCallsM, bitrate,
                               IptvConfig.GetUseBytes() ? "B" : "bit");
  filteredDataM = numberOfCallsM = 0;
  return s;
}

void cIptvSectionStatistics::AddSectionStatistic(long bytesP, long callsP)
{
  //debug("cIptvSectionStatistics::%s(%ld, %ld)", __FUNCTION__, bytesP, callsP); 
  cMutexLock MutexLock(&mutexM);
  filteredDataM += bytesP;
  numberOfCallsM += callsP;
}

// --- cIptvPidStatistics ----------------------------------------------------

// Device statistics class
cIptvPidStatistics::cIptvPidStatistics()
: timerM(),
  mutexM()
{
  debug("cIptvPidStatistics::%s()", __FUNCTION__);
  memset(mostActivePidsM, 0, sizeof(mostActivePidsM));
}

cIptvPidStatistics::~cIptvPidStatistics()
{
  debug("cIptvPidStatistics::%s()", __FUNCTION__);
}

cString cIptvPidStatistics::GetPidStatistic()
{
  //debug("cIptvPidStatistics::%s()", __FUNCTION__);
  cMutexLock MutexLock(&mutexM);
  uint64_t elapsed = timerM.Elapsed(); /* in milliseconds */
  timerM.Set();
  cString s("Active pids:\n");
  for (unsigned int i = 0; i < IPTV_STATS_ACTIVE_PIDS_COUNT; ++i) {
      if (mostActivePidsM[i].pid) {
         long bitrate = elapsed ? (long)(1000.0L * mostActivePidsM[i].DataAmount / KILOBYTE(1) / elapsed) : 0L;
         if (!IptvConfig.GetUseBytes())
            bitrate *= 8;
         s = cString::sprintf("%sPid %d: %4d (%4ld k%s/s)\n", *s, i,
                              mostActivePidsM[i].pid, bitrate,
                              IptvConfig.GetUseBytes() ? "B" : "bit");
         }
      }
  memset(mostActivePidsM, 0, sizeof(mostActivePidsM));
  return s;
}

int cIptvPidStatistics::SortPids(const void* data1P, const void* data2P)
{
  //debug("cIptvPidStatistics::%s()", __FUNCTION__);
  const pidStruct *comp1 = reinterpret_cast<const pidStruct*>(data1P);
  const pidStruct *comp2 = reinterpret_cast<const pidStruct*>(data2P);
  if (comp1->DataAmount > comp2->DataAmount)
     return -1;
  if (comp1->DataAmount < comp2->DataAmount)
     return 1;
  return 0;
}

void cIptvPidStatistics::AddPidStatistic(u_short pidP, long payloadP)
{
  //debug("cIptvPidStatistics::%s(%ld, %ld)", __FUNCTION__, pidP, payloadP);
  cMutexLock MutexLock(&mutexM);
  const int numberOfElements = sizeof(mostActivePidsM) / sizeof(pidStruct);
  // If our statistic already is in the array, update it and quit
  for (int i = 0; i < numberOfElements; ++i) {
      if (mostActivePidsM[i].pid == pidP) {
         mostActivePidsM[i].DataAmount += payloadP;
         // Now re-sort the array and quit
         qsort(mostActivePidsM, numberOfElements, sizeof(pidStruct), SortPids);
         return;
         }
      }
  // Apparently our pid isn't in the array. Replace the last element with this
  // one if new payload is greater
  if (mostActivePidsM[numberOfElements - 1].DataAmount < payloadP) {
      mostActivePidsM[numberOfElements - 1].pid = pidP;
      mostActivePidsM[numberOfElements - 1].DataAmount = payloadP;
     // Re-sort
     qsort(mostActivePidsM, numberOfElements, sizeof(pidStruct), SortPids);
     }
}

// --- cIptvStreamerStatistics -----------------------------------------------

// Streamer statistics class
cIptvStreamerStatistics::cIptvStreamerStatistics()
: dataBytesM(0),
  timerM(),
  mutexM()
{
  debug("cIptvStreamerStatistics::%s()", __FUNCTION__);
}

cIptvStreamerStatistics::~cIptvStreamerStatistics()
{
  debug("cIptvStreamerStatistics::%s()", __FUNCTION__);
}

cString cIptvStreamerStatistics::GetStreamerStatistic()
{
  //debug("cIptvStreamerStatistics::%s()", __FUNCTION__);
  cMutexLock MutexLock(&mutexM);
  uint64_t elapsed = timerM.Elapsed(); /* in milliseconds */
  timerM.Set();
  long bitrate = elapsed ? (long)(1000.0L * dataBytesM / KILOBYTE(1) / elapsed) : 0L;
  if (!IptvConfig.GetUseBytes())
     bitrate *= 8;
  cString s = cString::sprintf("%ld k%s/s", bitrate, IptvConfig.GetUseBytes() ? "B" : "bit");
  dataBytesM = 0;
  return s;
}

void cIptvStreamerStatistics::AddStreamerStatistic(long bytesP)
{
  //debug("cIptvStreamerStatistics::%s(%ld)", __FUNCTION__, bytesP);
  cMutexLock MutexLock(&mutexM);
  dataBytesM += bytesP;
}


// Buffer statistics class
cIptvBufferStatistics::cIptvBufferStatistics()
: dataBytesM(0),
  freeSpaceM(0),
  usedSpaceM(0),
  timerM(),
  mutexM()
{
  debug("cIptvBufferStatistics::%s()", __FUNCTION__);
}

cIptvBufferStatistics::~cIptvBufferStatistics()
{
  debug("cIptvBufferStatistics::%s()", __FUNCTION__);
}

cString cIptvBufferStatistics::GetBufferStatistic()
{
  //debug("cIptvBufferStatistics::%s()", __FUNCTION__);
  cMutexLock MutexLock(&mutexM);
  uint64_t elapsed = timerM.Elapsed(); /* in milliseconds */
  timerM.Set();
  long bitrate = elapsed ? (long)(1000.0L * dataBytesM / KILOBYTE(1) / elapsed) : 0L;
  long totalSpace = MEGABYTE(IptvConfig.GetTsBufferSize());
  float percentage = (float)((float)usedSpaceM / (float)totalSpace * 100.0);
  long totalKilos = totalSpace / KILOBYTE(1);
  long usedKilos = usedSpaceM / KILOBYTE(1);
  if (!IptvConfig.GetUseBytes()) {
     bitrate *= 8;
     totalKilos *= 8;
     usedKilos *= 8;
     }
  cString s = cString::sprintf("Buffer bitrate: %ld k%s/s\nBuffer usage: %ld/%ld k%s (%2.1f%%)\n", bitrate,
                               IptvConfig.GetUseBytes() ? "B" : "bit", usedKilos, totalKilos,
                               IptvConfig.GetUseBytes() ? "B" : "bit", percentage);
  dataBytesM = 0;
  usedSpaceM = 0;
  return s;
}

void cIptvBufferStatistics::AddBufferStatistic(long bytesP, long usedP)
{
  //debug("cIptvBufferStatistics::%s(%ld, %ld)", __FUNCTION__, bytesP, usedP);
  cMutexLock MutexLock(&mutexM);
  dataBytesM += bytesP;
  if (usedP > usedSpaceM)
     usedSpaceM = usedP;
}
