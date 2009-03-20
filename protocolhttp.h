/*
 * protocolhttp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
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
  bool GetHeaderLine(char* dest, unsigned int destLen, unsigned int &recvLen);
  bool ProcessHeaders(void);

public:
  cIptvProtocolHttp();
  virtual ~cIptvProtocolHttp();
  int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool Set(const char* Location, const int Parameter, const int Index);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLHTTP_H

