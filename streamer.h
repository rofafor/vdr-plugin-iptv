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
  cRingBufferLinear* ringBuffer;
  cCondWait sleep;
  unsigned char* packetBuffer;
  unsigned int packetBufferLen;
  cIptvProtocolIf* protocol;

protected:
  virtual void Action(void);

public:
  cIptvStreamer(cRingBufferLinear* RingBuffer, unsigned int PacketLen);
  virtual ~cIptvStreamer();
  bool Set(const char* Location, const int Parameter, const int Index, cIptvProtocolIf* Protocol);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_STREAMER_H
