/*
 * protocolext.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolext.h,v 1.9 2008/01/04 23:36:37 ajhseppa Exp $
 */

#ifndef __IPTV_PROTOCOLEXT_H
#define __IPTV_PROTOCOLEXT_H

#include <arpa/inet.h>
#include "protocolif.h"
#include "socket.h"

class cIptvProtocolExt : public cIptvUdpSocket, public cIptvProtocolIf {
private:
  int pid;
  char* listenAddr;
  char* scriptFile;
  int scriptParameter;

private:
  void TerminateScript(void);
  void ExecuteScript(void);

public:
  cIptvProtocolExt();
  virtual ~cIptvProtocolExt();
  int Read(unsigned char* *BufferAddr);
  bool Set(const char* Location, const int Parameter, const int Index);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLEXT_H

