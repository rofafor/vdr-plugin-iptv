/*
 * protocolfile.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolfile.h,v 1.3 2007/09/20 21:45:51 rahrenbe Exp $
 */

#ifndef __IPTV_PROTOCOLFILE_H
#define __IPTV_PROTOCOLFILE_H

#include <arpa/inet.h>
#include "protocolif.h"

class cIptvProtocolFile : public cIptvProtocolIf {
private:
  char* streamAddr;
  int streamPort;
  FILE* fileStream;
  unsigned char* readBuffer;
  bool fileActive;

private:
  bool OpenFile(void);
  void CloseFile(void);

public:
  cIptvProtocolFile();
  virtual ~cIptvProtocolFile();
  virtual int Read(unsigned char* *BufferAddr);
  virtual bool Set(const char* Address, const int Port);
  virtual bool Open(void);
  virtual bool Close(void);
};

#endif // __IPTV_PROTOCOLFILE_H

