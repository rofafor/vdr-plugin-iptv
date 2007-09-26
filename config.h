/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.5 2007/09/26 19:49:35 rahrenbe Exp $
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
  unsigned int rtpBufferSize;
  unsigned int httpBufferSize;
  unsigned int fileBufferSize;
  unsigned int maxBufferSize;
public:
  cIptvConfig();
  unsigned int GetTsBufferSize(void) { return tsBufferSize; }
  unsigned int GetTsBufferPrefillRatio(void) { return tsBufferPrefillRatio; }
  unsigned int GetUdpBufferSize(void) { return udpBufferSize; }
  unsigned int GetRtpBufferSize(void) { return rtpBufferSize; }
  unsigned int GetHttpBufferSize(void) { return httpBufferSize; }
  unsigned int GetFileBufferSize(void) { return fileBufferSize; }
  unsigned int GetMaxBufferSize(void) { return maxBufferSize; }
  void SetTsBufferSize(unsigned int Size) { tsBufferSize = Size; }
  void SetTsBufferPrefillRatio(unsigned int Ratio) { tsBufferPrefillRatio = Ratio; }
  void SetUdpBufferSize(unsigned int Size) { udpBufferSize = Size; }
  void SetRtpBufferSize(unsigned int Size) { rtpBufferSize = Size; }
  void SetHttpBufferSize(unsigned int Size) { httpBufferSize = Size; }
  void SetFileBufferSize(unsigned int Size) { fileBufferSize = Size; }
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
