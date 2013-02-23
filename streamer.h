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
#include <vdr/ringbuffer.h>

#include "protocolif.h"
#include "statistics.h"

class cIptvStreamer : public cThread, public cIptvStreamerStatistics {
private:
  cRingBufferLinear* ringBufferM;
  cCondWait sleepM;
  unsigned char* packetBufferM;
  unsigned int packetBufferLenM;
  cIptvProtocolIf* protocolM;

protected:
  virtual void Action(void);

public:
  cIptvStreamer(cRingBufferLinear* ringBufferP, unsigned int packetLenP);
  virtual ~cIptvStreamer();
  bool Set(const char* locationP, const int parameterP, const int indexP, cIptvProtocolIf* protocolP);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_STREAMER_H
