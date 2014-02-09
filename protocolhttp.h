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
  char* streamAddrM;
  char* streamPathM;
  int streamPortM;

private:
  bool Connect(void);
  bool Disconnect(void);
  bool GetHeaderLine(char* destP, unsigned int destLenP, unsigned int &recvLenP);
  bool ProcessHeaders(void);

public:
  cIptvProtocolHttp();
  virtual ~cIptvProtocolHttp();
  int Read(unsigned char* bufferAddrP, unsigned int bufferLenP);
  bool SetSource(const char* locationP, const int parameterP, const int indexP);
  bool SetPid(int pidP, int typeP, bool onP);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLHTTP_H

