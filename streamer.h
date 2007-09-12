/*
 * streamer.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.h,v 1.2 2007/09/12 18:33:56 ajhseppa Exp $
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
  int dataProtocol;
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
  enum {
    PROTOCOL_UDP,
    PROTOCOL_RTSP,
    PROTOCOL_HTTP
  };
  cIptvStreamer();
  cIptvStreamer(cRingBufferLinear* BufferPtr, cMutex* Mutex);
  virtual ~cIptvStreamer();
  virtual void Action();
  bool SetStream(const char* address, const int port, const int protocol);
  bool Activate();
  bool Deactivate();
};

#endif // __IPTV_STREAMER_H

