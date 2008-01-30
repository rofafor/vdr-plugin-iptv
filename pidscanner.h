/*
 * pidscanner.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: pidscanner.h,v 1.1 2008/01/30 22:41:59 rahrenbe Exp $
 */

#ifndef __PIDSCANNER_H
#define __PIDSCANNER_H

#include <vdr/channels.h>

class cPidScanner {
private:
  cChannel channel;
  bool process;
  int Vpid;
  int Apid;
  int numVpids;
  int numApids;

public:
  cPidScanner(void);
  ~cPidScanner();
  void Process(const uint8_t* buf);
  void SetChannel(const cChannel *Channel);
};

#endif // __PIDSCANNER_H
