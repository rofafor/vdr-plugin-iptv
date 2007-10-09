/*
 * streamer.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.h,v 1.11 2007/10/09 22:12:17 rahrenbe Exp $
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
  bool Set(const char* Address, const int Port, cIptvProtocolIf* Protocol);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_STREAMER_H

