/*
 * streamer.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.h,v 1.3 2007/09/12 21:14:51 rahrenbe Exp $
 */

#ifndef __IPTV_STREAMER_H
#define __IPTV_STREAMER_H

#include <arpa/inet.h>

#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

class cIptvStreamer : public cThread {
private:
  char stream[256];
  int socketDesc;
  int dataPort;
  struct sockaddr_in sa;
  cRingBufferLinear* pRingBuffer;
  unsigned char* pReceiveBuffer;
  unsigned int bufferSize;
  cMutex* mutex;
  bool socketActive;
  bool mcastActive;

  bool CheckAndCreateSocket(const int port);
  void CloseSocket();

public:
  cIptvStreamer();
  cIptvStreamer(cRingBufferLinear* BufferPtr, cMutex* Mutex);
  virtual ~cIptvStreamer();
  virtual void Action();
  bool SetStream(const char* address, const int port, const char* protocol);
  bool Activate();
  bool Deactivate();
};

#endif // __IPTV_STREAMER_H

