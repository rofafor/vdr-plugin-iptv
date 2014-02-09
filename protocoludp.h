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
  bool isIGMPv3M;
  char* sourceAddrM;
  char* streamAddrM;
  int streamPortM;

public:
  cIptvProtocolUdp();
  virtual ~cIptvProtocolUdp();
  int Read(unsigned char* bufferAddrP, unsigned int bufferLenP);
  bool SetSource(const char* locationP, const int parameterP, const int indexP);
  bool SetPid(int pidP, int typeP, bool onP);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLUDP_H

