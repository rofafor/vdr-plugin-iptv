/*
 * source.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_SOURCE_H
#define __IPTV_SOURCE_H

#include <vdr/menuitems.h>
#include <vdr/sourceparams.h>
#include "common.h"

class cIptvTransponderParameters
{
  friend class cIptvSourceParam;

private:
  int sidscan;
  int pidscan;
  int protocol;
  char address[MaxFileName];
  int parameter;

public:
  enum {
    eProtocolUDP,
    eProtocolHTTP,
    eProtocolFILE,
    eProtocolEXT,
    eProtocolCount
  };
  cIptvTransponderParameters(const char *Parameters = NULL);
  int SidScan(void) const { return sidscan; }
  int PidScan(void) const { return pidscan; }
  int Protocol(void) const { return protocol; }
  const char *Address(void) const { return address; }
  int Parameter(void) const { return parameter; }
  void SetSidScan(int SidScan) { sidscan = SidScan; }
  void SetPidScan(int PidScan) { pidscan = PidScan; }
  void SetProtocol(int Protocol) { protocol = Protocol; }
  void SetAddress(const char *Address) { strncpy(address, Address, sizeof(address)); }
  void SetParameter(int Parameter) { parameter = Parameter; }
  cString ToString(char Type) const;
  bool Parse(const char *s);
};

class cIptvSourceParam : public cSourceParam
{
private:
  int param;
  int nid;
  int tid;
  int rid;
  cChannel data;
  cIptvTransponderParameters itp;
  const char *protocols[cIptvTransponderParameters::eProtocolCount];

public:
  cIptvSourceParam(char Source, const char *Description);
  virtual void SetData(cChannel *Channel);
  virtual void GetData(cChannel *Channel);
  virtual cOsdItem *GetOsdItem(void);
};

#endif // __IPTV_SOURCE_H
