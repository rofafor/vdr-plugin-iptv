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

#include "log.h"

class cPidScanner {
private:
  enum {
    PIDSCANNER_APID_COUNT      = 5,    /* minimum count of audio pid samples for pid detection */
    PIDSCANNER_VPID_COUNT      = 5,    /* minimum count of video pid samples for pid detection */
    PIDSCANNER_PID_DELTA_COUNT = 100,  /* minimum count of pid samples for audio/video only pid detection */
    PIDSCANNER_TIMEOUT_IN_MS   = 15000 /* 15s timeout for detection */
  };
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
  void Open()  { debug1("%s", __PRETTY_FUNCTION__); timeoutM.Set(PIDSCANNER_TIMEOUT_IN_MS); }
  void Close() { debug1("%s", __PRETTY_FUNCTION__); timeoutM.Set(0); }
};

#endif // __PIDSCANNER_H
