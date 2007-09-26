/*
 * protocolrtp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolrtp.h,v 1.1 2007/09/26 19:49:35 rahrenbe Exp $
 */

#ifndef __IPTV_PROTOCOLRTP_H
#define __IPTV_PROTOCOLRTP_H

#include <arpa/inet.h>
#include "protocolif.h"

class cIptvProtocolRtp : public cIptvProtocolIf {
private:
  char* streamAddr;
  int streamPort;
  int socketDesc;
  unsigned char* readBuffer;
  struct sockaddr_in sockAddr;
  bool isActive;

private:
  bool OpenSocket(const int Port);
  void CloseSocket(void);
  bool JoinMulticast(void);
  bool DropMulticast(void);

public:
  cIptvProtocolRtp();
  virtual ~cIptvProtocolRtp();
  virtual int Read(unsigned char* *BufferAddr);
  virtual bool Set(const char* Address, const int Port);
  virtual bool Open(void);
  virtual bool Close(void);
};

#endif // __IPTV_PROTOCOLRTP_H

