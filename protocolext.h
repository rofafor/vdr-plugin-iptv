/*
 * protocolext.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolext.h,v 1.5 2007/10/19 22:18:55 rahrenbe Exp $
 */

#ifndef __IPTV_PROTOCOLEXT_H
#define __IPTV_PROTOCOLEXT_H

#include <arpa/inet.h>
#include "protocolif.h"

class cIptvProtocolExt : public cIptvProtocolIf {
private:
  int pid;
  char* listenAddr;
  int listenPort;
  char* scriptFile;
  int scriptParameter;
  int socketDesc;
  unsigned char* readBuffer;
  unsigned int readBufferLen;
  struct sockaddr_in sockAddr;
  bool isActive;

private:
  bool OpenSocket(void);
  void CloseSocket(void);
  void TerminateCommand(void);
  void ExecuteCommand(void);

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

