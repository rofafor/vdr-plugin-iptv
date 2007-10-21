/*
 * protocolhttp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolhttp.h,v 1.10 2007/10/21 13:31:21 ajhseppa Exp $
 */

#ifndef __IPTV_PROTOCOLHTTP_H
#define __IPTV_PROTOCOLHTTP_H

#include <arpa/inet.h>
#include "protocolif.h"
#include "socket.h"

class cIptvProtocolHttp : public cIptvSocket, public cIptvProtocolIf {
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
  virtual int Read(unsigned char* *BufferAddr);
  virtual bool Set(const char* Location, const int Parameter, const int Index);
  virtual bool Open(void);
  virtual bool Close(void);
  virtual cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLHTTP_H

