/*
 * protocolfile.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
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
  bool isActive;

private:
  bool OpenFile(void);
  void CloseFile(void);

public:
  cIptvProtocolFile();
  virtual ~cIptvProtocolFile();
  int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool Set(const char* Location, const int Parameter, const int Index);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLFILE_H

