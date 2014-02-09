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
  char* fileLocationM;
  int fileDelayM;
  FILE* fileStreamM;
  bool isActiveM;

private:
  bool OpenFile(void);
  void CloseFile(void);

public:
  cIptvProtocolFile();
  virtual ~cIptvProtocolFile();
  int Read(unsigned char* bufferAddrP, unsigned int bufferLenP);
  bool SetSource(const char* locationP, const int parameterP, const int indexP);
  bool SetPid(int pidP, int typeP, bool onP);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
};

#endif // __IPTV_PROTOCOLFILE_H

