/*
 * protocolfile.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolfile.h,v 1.6 2007/10/19 21:36:28 rahrenbe Exp $
 */

#ifndef __IPTV_PROTOCOLFILE_H
#define __IPTV_PROTOCOLFILE_H

#include <arpa/inet.h>
#include "protocolif.h"

class cIptvProtocolFile : public cIptvProtocolIf {
private:
  char* fileLocation;
  int fileDelay;
  FILE* fileStream;
  unsigned char* readBuffer;
  unsigned int readBufferLen;
  bool isActive;

private:
  bool OpenFile(void);
  void CloseFile(void);

public:
  cIptvProtocolFile();
  virtual ~cIptvProtocolFile();
  virtual int Read(unsigned char* *BufferAddr);
  virtual bool Set(const char* Location, const int Parameter);
  virtual bool Open(void);
  virtual bool Close(void);
  virtual cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLFILE_H

