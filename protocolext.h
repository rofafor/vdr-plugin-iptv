/*
 * protocolext.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolext.h,v 1.7 2007/10/21 13:31:21 ajhseppa Exp $
 */

#ifndef __IPTV_PROTOCOLEXT_H
#define __IPTV_PROTOCOLEXT_H

#include <arpa/inet.h>
#include "protocolif.h"
#include "socket.h"

class cIptvProtocolExt : public cIptvSocket, public cIptvProtocolIf {
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
  virtual int Read(unsigned char* *BufferAddr);
  virtual bool Set(const char* Location, const int Parameter, const int Index);
  virtual bool Open(void);
  virtual bool Close(void);
  virtual cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLEXT_H

