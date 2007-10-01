/*
 * sidscanner.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: sidscanner.h,v 1.1 2007/10/01 18:14:57 rahrenbe Exp $
 */

#ifndef __SIDSCANNER_H
#define __SIDSCANNER_H

#include <vdr/channels.h>
#include <vdr/filter.h>

class cSidScanner : public cFilter {
private:
  cChannel channel;

protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);

public:
  cSidScanner(void);
  virtual void SetStatus(bool On);
  void SetChannel(const cChannel *Channel);
};

#endif // __SIDSCANNER_H
