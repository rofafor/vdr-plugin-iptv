/*
 * streamer.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.h,v 1.6 2007/09/14 15:44:25 rahrenbe Exp $
 */

#ifndef __IPTV_STREAMER_H
#define __IPTV_STREAMER_H

#include <arpa/inet.h>

#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

#include "protocolif.h"

class cIptvStreamer : public cThread {
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
};

#endif // __IPTV_STREAMER_H

