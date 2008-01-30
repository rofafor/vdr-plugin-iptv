/*
 * streamer.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.h,v 1.14 2008/01/30 21:57:33 rahrenbe Exp $
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
  cMutex* mutex;
  unsigned char* readBuffer;
  unsigned int readBufferLen;
  cIptvProtocolIf* protocol;

public:
  cIptvStreamer(cRingBufferLinear* RingBuffer, cMutex* Mutex);
  virtual ~cIptvStreamer();
  virtual void Action(void);
  bool Set(const char* Location, const int Parameter, const int Index, cIptvProtocolIf* Protocol);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_STREAMER_H
