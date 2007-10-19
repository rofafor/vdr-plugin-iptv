/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.14 2007/10/19 22:18:55 rahrenbe Exp $
 */

#ifndef __IPTV_CONFIG_H
#define __IPTV_CONFIG_H

#include <vdr/menuitems.h>
#include "common.h"
#include "config.h"

class cIptvConfig
{
protected:
  unsigned int readBufferTsCount;
  unsigned int tsBufferSize;
  unsigned int tsBufferPrefillRatio;
  unsigned int useBytes;
  unsigned int sectionFiltering;
  unsigned int sidScanning;
  unsigned int extListenPortBase;
  int disabledFilters[SECTION_FILTER_TABLE_SIZE];

public:
  cIptvConfig();
  unsigned int GetReadBufferTsCount(void) { return readBufferTsCount; }
  unsigned int GetTsBufferSize(void) { return tsBufferSize; }
  unsigned int GetTsBufferPrefillRatio(void) { return tsBufferPrefillRatio; }
  unsigned int GetUseBytes(void) { return useBytes; }
  unsigned int GetSectionFiltering(void) { return sectionFiltering; }
  unsigned int GetSidScanning(void) { return sidScanning; }
  unsigned int GetExtListenPortBase(void) { return extListenPortBase; }
  unsigned int GetDisabledFiltersCount(void);
  int GetDisabledFilters(unsigned int Index);
  void SetTsBufferSize(unsigned int Size) { tsBufferSize = Size; }
  void SetTsBufferPrefillRatio(unsigned int Ratio) { tsBufferPrefillRatio = Ratio; }
  void SetUseBytes(unsigned int On) { useBytes = On; }
  void SetSectionFiltering(unsigned int On) { sectionFiltering = On; }
  void SetSidScanning(unsigned int On) { sidScanning = On; }
  void SetExtListenPortBase(unsigned int PortNumber) { extListenPortBase = PortNumber; }
  void SetDisabledFilters(unsigned int Index, int Number);
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
