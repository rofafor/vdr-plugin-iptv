/*
 * protocoludp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_PROTOCOLUDP_H
#define __IPTV_PROTOCOLUDP_H

#include <arpa/inet.h>
#include "protocolif.h"
#include "socket.h"

class cIptvProtocolUdp : public cIptvUdpSocket, public cIptvProtocolIf {
private:
  char* streamAddr;

private:
  bool JoinMulticast(void);
  bool DropMulticast(void);

public:
  cIptvProtocolUdp();
  virtual ~cIptvProtocolUdp();
  int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool Set(const char* Location, const int Parameter, const int Index);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLUDP_H

