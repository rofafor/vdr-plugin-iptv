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
  int sidScanM;
  int pidScanM;
  int protocolM;
  char addressM[NAME_MAX + 1];
  int parameterM;

public:
  enum {
    eProtocolUDP,
    eProtocolCURL,
    eProtocolHTTP,
    eProtocolFILE,
    eProtocolEXT,
    eProtocolCount
  };
  cIptvTransponderParameters(const char *parametersP = NULL);
  int SidScan(void) const { return sidScanM; }
  int PidScan(void) const { return pidScanM; }
  int Protocol(void) const { return protocolM; }
  const char *Address(void) const { return addressM; }
  int Parameter(void) const { return parameterM; }
  void SetSidScan(int sidScanP) { sidScanM = sidScanP; }
  void SetPidScan(int pidScanP) { pidScanM = pidScanP; }
  void SetProtocol(int protocolP) { protocolM = protocolP; }
  void SetAddress(const char *addressP) { strncpy(addressM, addressP, sizeof(addressM)); }
  void SetParameter(int parameterP) { parameterM = parameterP; }
  cString ToString(char typeP) const;
  bool Parse(const char *strP);
};

class cIptvSourceParam : public cSourceParam
{
private:
  int paramM;
  int ridM;
  cChannel dataM;
  cIptvTransponderParameters itpM;
  const char *protocolsM[cIptvTransponderParameters::eProtocolCount];

private:
  static const char *allowedProtocolCharsS;

public:
  cIptvSourceParam(char sourceP, const char *descriptionP);
  virtual void SetData(cChannel *channelP);
  virtual void GetData(cChannel *channelP);
  virtual cOsdItem *GetOsdItem(void);
};

#endif // __IPTV_SOURCE_H
