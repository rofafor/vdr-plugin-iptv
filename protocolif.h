/*
 * protocolif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_PROTOCOLIF_H
#define __IPTV_PROTOCOLIF_H

class cIptvProtocolIf {
public:
  cIptvProtocolIf() {}
  virtual ~cIptvProtocolIf() {}
  virtual int Read(unsigned char* bufferAddrP, unsigned int bufferLenP) = 0;
  virtual bool SetSource(const char* locationP, const int parameterP, const int indexP) = 0;
  virtual bool SetPid(int pidP, int typeP, bool onP) = 0;
  virtual bool Open(void) = 0;
  virtual bool Close(void) = 0;
  virtual cString GetInformation(void) = 0;

private:
  cIptvProtocolIf(const cIptvProtocolIf&);
  cIptvProtocolIf& operator=(const cIptvProtocolIf&);
};

#endif // __IPTV_PROTOCOLIF_H
