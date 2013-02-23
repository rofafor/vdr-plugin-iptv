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
  cTimeMs timeoutM;
  tChannelID channelIdM;
  bool processM;
  int vPidM;
  int aPidM;
  int numVpidsM;
  int numApidsM;

public:
  cPidScanner(void);
  ~cPidScanner();
  void SetChannel(const tChannelID &channelIdP);
  void Process(const uint8_t* bufP);
};

#endif // __PIDSCANNER_H
