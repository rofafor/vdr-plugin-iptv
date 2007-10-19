/*
 * protocoludp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocoludp.h,v 1.10 2007/10/19 22:18:55 rahrenbe Exp $
 */

#ifndef __IPTV_PROTOCOLUDP_H
#define __IPTV_PROTOCOLUDP_H

#include <arpa/inet.h>
#include "protocolif.h"

class cIptvProtocolUdp : public cIptvProtocolIf {
private:
  char* streamAddr;
  int streamPort;
  int socketDesc;
  unsigned char* readBuffer;
  unsigned int readBufferLen;
  struct sockaddr_in sockAddr;
  bool isActive;

private:
  bool OpenSocket(const int Port);
  void CloseSocket(void);
  bool JoinMulticast(void);
  bool DropMulticast(void);

public:
  cIptvProtocolUdp();
  virtual ~cIptvProtocolUdp();
  virtual int Read(unsigned char* *BufferAddr);
  virtual bool Set(const char* Location, const int Parameter, const int Index);
  virtual bool Open(void);
  virtual bool Close(void);
  virtual cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLUDP_H

