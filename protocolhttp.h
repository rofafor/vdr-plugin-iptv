/*
 * protocolhttp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolhttp.h,v 1.7 2007/10/07 22:54:09 rahrenbe Exp $
 */

#ifndef __IPTV_PROTOCOLHTTP_H
#define __IPTV_PROTOCOLHTTP_H

#include <arpa/inet.h>
#include "protocolif.h"

class cIptvProtocolHttp : public cIptvProtocolIf {
private:
  char* streamAddr;
  char* streamPath;
  int streamPort;
  int socketDesc;
  unsigned char* readBuffer;
  unsigned int readBufferLen;
  struct sockaddr_in sockAddr;
  bool isActive;

private:
  bool OpenSocket(const int Port);
  void CloseSocket(void);
  bool Connect(void);
  bool Disconnect(void);
  bool GetHeaderLine(char* dest, unsigned int destLen,
		     unsigned int &recvLen);
  bool ProcessHeaders(void);

public:
  cIptvProtocolHttp();
  virtual ~cIptvProtocolHttp();
  virtual int Read(unsigned char* *BufferAddr);
  virtual bool Set(const char* Address, const int Port);
  virtual bool Open(void);
  virtual bool Close(void);
  virtual cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLHTTP_H

