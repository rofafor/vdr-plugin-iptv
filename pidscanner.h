/*
 * pidscanner.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: pidscanner.h,v 1.3 2008/02/02 20:51:47 rahrenbe Exp $
 */

#ifndef __PIDSCANNER_H
#define __PIDSCANNER_H

#include <vdr/tools.h>
#include <vdr/channels.h>

class cPidScanner {
private:
  cTimeMs timeout;
  cChannel channel;
  bool process;
  int Vpid;
  int Apid;
  int numVpids;
  int numApids;

public:
  cPidScanner(void);
  ~cPidScanner();
  void SetChannel(const cChannel *Channel);
  void Process(const uint8_t* buf);
};

#endif // __PIDSCANNER_H
