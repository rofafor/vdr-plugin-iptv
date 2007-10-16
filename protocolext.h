/*
 * protocolext.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolext.h,v 1.2 2007/10/16 22:13:44 rahrenbe Exp $
 */

#ifndef __IPTV_PROTOCOLEXT_H
#define __IPTV_PROTOCOLEXT_H

#include <arpa/inet.h>
#include "protocolif.h"

class cIptvProtocolExt : public cIptvProtocolIf {
private:
  char* listenAddr;
  int listenPort;
  char* streamAddr;
  int socketDesc;
  unsigned char* readBuffer;
  unsigned int readBufferLen;
  struct sockaddr_in sockAddr;
  bool isActive;

private:
  bool OpenSocket(void);
  void CloseSocket(void);

public:
  cIptvProtocolExt();
  virtual ~cIptvProtocolExt();
  virtual int Read(unsigned char* *BufferAddr);
  virtual bool Set(const char* Address, const int Port);
  virtual bool Open(void);
  virtual bool Close(void);
  virtual cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLEXT_H

