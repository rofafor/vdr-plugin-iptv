/*
 * sidfinder.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: sidfinder.h,v 1.1 2007/09/28 23:23:12 rahrenbe Exp $
 */

#ifndef __SIDFINDER_H
#define __SIDFINDER_H

#include <vdr/channels.h>
#include <vdr/filter.h>

class cSidFinder : public cFilter {
private:
  cChannel channel;

protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);

public:
  cSidFinder(void);
  virtual void SetStatus(bool On);
  void SetChannel(const cChannel *Channel);
};

#endif // __SIDFINDER_H
