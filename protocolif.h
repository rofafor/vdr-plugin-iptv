/*
 * protocolif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolif.h,v 1.8 2008/01/30 21:57:33 rahrenbe Exp $
 */

#ifndef __IPTV_PROTOCOLIF_H
#define __IPTV_PROTOCOLIF_H

class cIptvProtocolIf {
public:
  cIptvProtocolIf() {}
  virtual ~cIptvProtocolIf() {}
  virtual int Read(unsigned char* *BufferAddr) = 0;
  virtual bool Set(const char* Location, const int Parameter, const int Index) = 0;
  virtual bool Open(void) = 0;
  virtual bool Close(void) = 0;
  virtual cString GetInformation(void) = 0;

private:
  cIptvProtocolIf(const cIptvProtocolIf&);
  cIptvProtocolIf& operator=(const cIptvProtocolIf&);
};

#endif // __IPTV_PROTOCOLIF_H
