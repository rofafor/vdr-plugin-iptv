/*
 * streamer.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.h,v 1.5 2007/09/13 16:58:22 rahrenbe Exp $
 */

#ifndef __IPTV_STREAMER_H
#define __IPTV_STREAMER_H

#include <arpa/inet.h>

#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

class cIptvStreamer : public cThread {
private:
  char* streamAddr;
  int streamPort;
  int socketDesc;
  struct sockaddr_in sockAddr;
  cRingBufferLinear* pRingBuffer;
  unsigned char* pReceiveBuffer;
  unsigned int bufferSize;
  cMutex* mutex;
  bool mcastActive;

private:
  bool OpenSocket(const int port);
  void CloseSocket(void);
  bool JoinMulticast(void);
  bool DropMulticast(void);

public:
  cIptvStreamer();
  cIptvStreamer(cRingBufferLinear* Buffer, cMutex* Mutex);
  virtual ~cIptvStreamer();
  virtual void Action(void);
  bool SetStream(const char* address, const int port, const char* protocol);
  bool OpenStream(void);
  bool CloseStream(void);
};

#endif // __IPTV_STREAMER_H

