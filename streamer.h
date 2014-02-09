/*
 * streamer.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_STREAMER_H
#define __IPTV_STREAMER_H

#include <arpa/inet.h>

#include <vdr/thread.h>

#include "deviceif.h"
#include "protocolif.h"
#include "statistics.h"

class cIptvStreamer : public cThread, public cIptvStreamerStatistics {
private:
  cCondWait sleepM;
  cIptvDeviceIf* deviceM;
  unsigned char* packetBufferM;
  unsigned int packetBufferLenM;
  cIptvProtocolIf* protocolM;

protected:
  virtual void Action(void);

public:
  cIptvStreamer(cIptvDeviceIf &deviceP, unsigned int packetLenP);
  virtual ~cIptvStreamer();
  bool SetSource(const char* locationP, const int parameterP, const int indexP, cIptvProtocolIf* protocolP);
  bool SetPid(int pidP, int typeP, bool onP);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_STREAMER_H
