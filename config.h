/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.9 2007/09/30 21:38:31 rahrenbe Exp $
 */

#ifndef __IPTV_CONFIG_H
#define __IPTV_CONFIG_H

#include <vdr/menuitems.h>
#include "config.h"

class cIptvConfig
{
protected:
  unsigned int readBufferTsCount;
  unsigned int tsBufferSize;
  unsigned int tsBufferPrefillRatio;
  unsigned int sectionFiltering;
  unsigned int sidScanning;

public:
  cIptvConfig();
  unsigned int GetReadBufferTsCount(void) { return readBufferTsCount; }
  unsigned int GetTsBufferSize(void) { return tsBufferSize; }
  unsigned int GetTsBufferPrefillRatio(void) { return tsBufferPrefillRatio; }
  unsigned int GetSectionFiltering(void) { return sectionFiltering; }
  unsigned int GetSidScanning(void) { return sidScanning; }
  void SetTsBufferSize(unsigned int Size) { tsBufferSize = Size; }
  void SetTsBufferPrefillRatio(unsigned int Ratio) { tsBufferPrefillRatio = Ratio; }
  void SetSectionFiltering(unsigned int On) { sectionFiltering = On; }
  void SetSidScanning(unsigned int On) { sidScanning = On; }
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
