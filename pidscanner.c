/*
 * pidscanner.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: pidscanner.c,v 1.2 2008/02/01 21:54:24 rahrenbe Exp $
 */

#include "common.h"
#include "pidscanner.h"

#define PIDSCANNER_TIMEOUT_IN_MS 60000 /* 60s */

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

void cPidScanner::Process(const uint8_t* buf)
{
  //debug("cPidScanner::Process()\n");
  if (!process)
     return;

  // Stop scanning after defined timeout
  if (timeout.TimedOut())
     process = false;

  if (buf[0] != 0x47) {
     error("Not TS packet: 0x%X\n", buf[0]);
     return;
     }

  // Found TS packet
  int pid = ts_pid(buf);
  int xpid = (buf[1] << 8 | buf[2]);

  // count == 0 if no payload or out of range
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
                 Apid = pid;
                 numApids = 1;
                 }
              else if (pid == Apid)
                 ++numApids;
              }
           else if ((sid >= 0xE0) && (sid <= 0xEF)) {
              if (pid < Vpid) {
                 Vpid = pid;
                 numVpids = 1;
                 }
              else if (pid == Vpid)
                 ++numVpids;
              }
           }
        if (numVpids > 10 && numApids > 5) {
           if (!Channels.Lock(true, 10))
              return;
	   debug("cPidScanner::Process(): Vpid=0x%04X, Apid=0x%04X\n", Vpid, Apid);
           cChannel *IptvChannel = Channels.GetByChannelID(channel.GetChannelID());
           int Apids[MAXAPIDS + 1] = { 0 }; // these lists are zero-terminated
           int Dpids[MAXDPIDS + 1] = { 0 };
           int Spids[MAXSPIDS + 1] = { 0 };
           char ALangs[MAXAPIDS][MAXLANGCODE2] = { "" };
           char DLangs[MAXDPIDS][MAXLANGCODE2] = { "" };
           char SLangs[MAXSPIDS][MAXLANGCODE2] = { "" };
           int Ppid = IptvChannel->Ppid();
           int Tpid = IptvChannel->Tpid();
           Apids[0] = Apid;
           for (unsigned int i = 1; i < MAXAPIDS; ++i)
               Apids[i] = IptvChannel->Apid(i);
           for (unsigned int i = 0; i < MAXDPIDS; ++i)
               Dpids[i] = IptvChannel->Dpid(i);
           for (unsigned int i = 0; i < MAXSPIDS; ++i)
               Spids[i] = IptvChannel->Spid(i);
           IptvChannel->SetPids(Vpid, Ppid, Apids, ALangs, Dpids, DLangs, Spids, SLangs, Tpid);
           Channels.Unlock();
	   process = false;
           }
        }
     }
}

void cPidScanner::SetChannel(const cChannel *Channel)
{
  if (Channel) {
     debug("cPidScanner::SetChannel(): %s\n", Channel->PluginParam());
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
