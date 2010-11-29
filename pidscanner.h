/*
 * pidscanner.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __PIDSCANNER_H
#define __PIDSCANNER_H

#include <vdr/tools.h>
#include <vdr/channels.h>

class cPidScanner {
private:
  cTimeMs timeout;
  tChannelID channelId;
  bool process;
  int Vpid;
  int Apid;
  int numVpids;
  int numApids;

public:
  cPidScanner(void);
  ~cPidScanner();
  void SetChannel(const tChannelID &ChannelId);
  void Process(const uint8_t* buf);
};

#endif // __PIDSCANNER_H
