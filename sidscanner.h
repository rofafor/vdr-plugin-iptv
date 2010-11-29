/*
 * sidscanner.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __SIDSCANNER_H
#define __SIDSCANNER_H

#include <vdr/channels.h>
#include <vdr/filter.h>

class cSidScanner : public cFilter {
private:
  tChannelID channelId;
  bool sidFound;
  bool nidFound;
  bool tidFound;

protected:
  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);
  virtual void SetStatus(bool On);

public:
  cSidScanner(void);
  ~cSidScanner();
  void SetChannel(const tChannelID &ChannelId);
  void Open() { SetStatus(true); }
  void Close() { SetStatus(false); }
};

#endif // __SIDSCANNER_H
