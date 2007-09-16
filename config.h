/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.3 2007/09/16 13:38:20 rahrenbe Exp $
 */

#ifndef __IPTV_CONFIG_H
#define __IPTV_CONFIG_H

#include <vdr/menuitems.h>
#include "config.h"

class cIptvConfig
{
protected:
  unsigned int tsBufferSize;
  unsigned int tsBufferPrefillRatio;
  unsigned int udpBufferSize;
  unsigned int httpBufferSize;
  unsigned int fileBufferSize;
public:
  cIptvConfig();
  unsigned int GetTsBufferSize(void) { return tsBufferSize; }
  unsigned int GetTsBufferPrefillRatio(void) { return tsBufferPrefillRatio; }
  unsigned int GetUdpBufferSize(void) { return udpBufferSize; }
  unsigned int GetHttpBufferSize(void) { return httpBufferSize; }
  unsigned int GetFileBufferSize(void) { return fileBufferSize; }
  void SetTsBufferSize(unsigned int Size) { tsBufferSize = Size; }
  void SetTsBufferPrefillRatio(unsigned int Ratio) { tsBufferPrefillRatio = Ratio; }
  void SetUdpBufferSize(unsigned int Size) { udpBufferSize = Size; }
  void SetHttpBufferSize(unsigned int Size) { httpBufferSize = Size; }
  void SetFileBufferSize(unsigned int Size) { fileBufferSize = Size; }
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
