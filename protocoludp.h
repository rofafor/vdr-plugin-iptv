/*
 * protocoludp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocoludp.h,v 1.1 2007/09/14 15:44:25 rahrenbe Exp $
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
  struct sockaddr_in sockAddr;
  bool mcastActive;

private:
  bool OpenSocket(const int Port);
  void CloseSocket(void);
  bool JoinMulticast(void);
  bool DropMulticast(void);

public:
  cIptvProtocolUdp();
  virtual ~cIptvProtocolUdp();
  virtual int Read(unsigned char *Buffer, int Len);
  virtual bool Set(const char* Address, const int Port);
  virtual bool Open(void);
  virtual bool Close(void);
};

#endif // __IPTV_PROTOCOLUDP_H

