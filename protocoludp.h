/*
 * protocoludp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocoludp.h,v 1.6 2007/09/28 16:44:59 rahrenbe Exp $
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
  virtual bool Set(const char* Address, const int Port);
  virtual bool Open(void);
  virtual bool Close(void);
};

#endif // __IPTV_PROTOCOLUDP_H

