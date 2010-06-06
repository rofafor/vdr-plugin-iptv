/*
 * pidscanner.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "pidscanner.h"

#define PIDSCANNER_TIMEOUT_IN_MS   15000 /* 15s timeout for detection */
#define PIDSCANNER_APID_COUNT      5     /* minimum count of audio pid samples for pid detection */
#define PIDSCANNER_VPID_COUNT      5     /* minimum count of video pid samples for pid detection */
#define PIDSCANNER_PID_DELTA_COUNT 100   /* minimum count of pid samples for audio/video only pid detection */

cPidScanner::cPidScanner(void) 
: timeout(0),
  process(true),
  Vpid(0xFFFF),
  Apid(0xFFFF),
  numVpids(0),
  numApids(0)
{
  debug("cPidScanner::cPidScanner()\n");
  channel = cChannel();
}

cPidScanner::~cPidScanner() 
{
  debug("cPidScanner::~cPidScanner()\n");
}

void cPidScanner::SetChannel(const cChannel *Channel)
{
  if (Channel) {
     debug("cPidScanner::SetChannel(): %s\n", Channel->Parameters());
     channel = *Channel;
     }
  else {
     debug("cPidScanner::SetChannel()\n");
     channel = cChannel();
     }
  Vpid = 0xFFFF;
  numVpids = 0;
  Apid = 0xFFFF;
  numApids = 0;
  process = true;
  timeout.Set(PIDSCANNER_TIMEOUT_IN_MS);
}

void cPidScanner::Process(const uint8_t* buf)
{
  //debug("cPidScanner::Process()\n");
  if (!process)
     return;

  // Stop scanning after defined timeout
  if (timeout.TimedOut()) {
     debug("cPidScanner::Process: Timed out determining pids\n");
     process = false;
  }

  // Verify TS packet
  if (buf[0] != 0x47) {
     error("Not TS packet: 0x%X\n", buf[0]);
     return;
     }

  // Found TS packet
  int pid = ts_pid(buf);
  int xpid = (buf[1] << 8 | buf[2]);

  // Check if payload available
  uint8_t count = payload(buf);
  if (count == 0)
     return;

  if (xpid & 0x4000) {
     // Stream start (Payload Unit Start Indicator)
     uchar *d = (uint8_t*)buf;
     d += 4;
     // pointer to payload
     if (buf[3] & 0x20)
        d += d[0] + 1;
     // Skip adaption field
     if (buf[3] & 0x10) {
        // Payload present
        if ((d[0] == 0) && (d[1] == 0) && (d[2] == 1)) {
           // PES packet start
           int sid = d[3];
           // Stream ID
           if ((sid >= 0xC0) && (sid <= 0xDF)) {
              if (pid < Apid) {
                 debug("cPidScanner::Process: Found lower Apid: 0x%X instead of 0x%X\n", pid, Apid);
                 Apid = pid;
                 numApids = 1;
                 }
              else if (pid == Apid) {
                 ++numApids;
                 debug("cPidScanner::Process: Incrementing Apids, now at %d\n", numApids);
                 }
              }
           else if ((sid >= 0xE0) && (sid <= 0xEF)) {
              if (pid < Vpid) {
                 debug("cPidScanner::Process: Found lower Vpid: 0x%X instead of 0x%X\n", pid, Vpid);
                 Vpid = pid;
                 numVpids = 1;
                 }
              else if (pid == Vpid) {
                 ++numVpids;
                 debug("cPidScanner::Process: Incrementing Vpids, now at %d\n", numVpids);
                 }
              }
           }
        if (((numVpids >= PIDSCANNER_VPID_COUNT) && (numApids >= PIDSCANNER_APID_COUNT)) ||
            (abs(numApids - numVpids) >= PIDSCANNER_PID_DELTA_COUNT)) {
           // Lock channels for pid updates
           if (!Channels.Lock(true, 10)) {
              timeout.Set(PIDSCANNER_TIMEOUT_IN_MS);
              return;
              }
           cChannel *IptvChannel = Channels.GetByChannelID(channel.GetChannelID());
           if (IptvChannel) {
              int Apids[MAXAPIDS + 1] = { 0 }; // these lists are zero-terminated
              int Atypes[MAXAPIDS + 1] = { 0 };
              int Dpids[MAXDPIDS + 1] = { 0 };
              int Dtypes[MAXDPIDS + 1] = { 0 };
              int Spids[MAXSPIDS + 1] = { 0 };
              char ALangs[MAXAPIDS][MAXLANGCODE2] = { "" };
              char DLangs[MAXDPIDS][MAXLANGCODE2] = { "" };
              char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
              int Ppid = IptvChannel->Ppid();
              int Tpid = IptvChannel->Tpid();
              bool foundApid = false;
              if (numVpids < PIDSCANNER_VPID_COUNT)
                 Vpid = 0; // No detected video pid
              else if (numApids < PIDSCANNER_APID_COUNT)
                 Apid = 0; // No detected audio pid
              for (unsigned int i = 1; i < MAXAPIDS; ++i) {
                  Apids[i] = IptvChannel->Apid(i);
                  Atypes[i] = IptvChannel->Atype(i);
                  if (Apids[i] && (Apids[i] == Apid))
                     foundApid = true;
                  }
              if (!foundApid) {
                 Apids[0] = Apid;
                 Atypes[0] = 4;
                 }
              for (unsigned int i = 0; i < MAXDPIDS; ++i) {
                  Dpids[i] = IptvChannel->Dpid(i);
                  Dtypes[i] = IptvChannel->Dtype(i);
                  }
              for (unsigned int i = 0; i < MAXSPIDS; ++i)
                  Spids[i] = IptvChannel->Spid(i);
              debug("cPidScanner::Process(): Vpid=0x%04X, Apid=0x%04X\n", Vpid, Apid);
              int Vtype = IptvChannel->Vtype();
              IptvChannel->SetPids(Vpid, Ppid, Vtype, Apids, Atypes, ALangs, Dpids, Dtypes, DLangs, Spids, SLangs, Tpid);
              }
           Channels.Unlock();
           process = false;
           }
        }
     }
}
