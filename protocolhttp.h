/*
 * protocolhttp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolhttp.h,v 1.11 2007/10/21 17:32:43 ajhseppa Exp $
 */

#ifndef __IPTV_PROTOCOLHTTP_H
#define __IPTV_PROTOCOLHTTP_H

#include <arpa/inet.h>
#include "protocolif.h"
#include "socket.h"

class cIptvProtocolHttp : public cIptvTcpSocket, public cIptvProtocolIf {
private:
  char* streamAddr;
  char* streamPath;

private:
  bool Connect(void);
  bool Disconnect(void);
  bool GetHeaderLine(char* dest, unsigned int destLen,
		     unsigned int &recvLen);
  bool ProcessHeaders(void);

public:
  cIptvProtocolHttp();
  virtual ~cIptvProtocolHttp();
  int Read(unsigned char* *BufferAddr);
  virtual bool Set(const char* Location, const int Parameter, const int Index);
  virtual bool Open(void);
  virtual bool Close(void);
  virtual cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLHTTP_H

