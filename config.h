/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.17 2007/10/28 16:22:44 rahrenbe Exp $
 */

#ifndef __IPTV_CONFIG_H
#define __IPTV_CONFIG_H

#include <vdr/menuitems.h>
#include "common.h"

class cIptvConfig
{
private:
  unsigned int readBufferTsCount;
  unsigned int tsBufferSize;
  unsigned int tsBufferPrefillRatio;
  unsigned int extProtocolBasePort;
  unsigned int useBytes;
  unsigned int sectionFiltering;
  unsigned int sidScanning;
  int disabledFilters[SECTION_FILTER_TABLE_SIZE];
  char configDirectory[255];

public:
  cIptvConfig();
  unsigned int GetReadBufferTsCount(void) { return readBufferTsCount; }
  unsigned int GetTsBufferSize(void) { return tsBufferSize; }
  unsigned int GetTsBufferPrefillRatio(void) { return tsBufferPrefillRatio; }
  unsigned int GetExtProtocolBasePort(void) { return extProtocolBasePort; }
  unsigned int GetUseBytes(void) { return useBytes; }
  unsigned int GetSectionFiltering(void) { return sectionFiltering; }
  unsigned int GetSidScanning(void) { return sidScanning; }
  const char *GetConfigDirectory(void) { return configDirectory; }
  unsigned int GetDisabledFiltersCount(void);
  int GetDisabledFilters(unsigned int Index);
  void SetTsBufferSize(unsigned int Size) { tsBufferSize = Size; }
  void SetTsBufferPrefillRatio(unsigned int Ratio) { tsBufferPrefillRatio = Ratio; }
  void SetExtProtocolBasePort(unsigned int PortNumber) { extProtocolBasePort = PortNumber; }
  void SetUseBytes(unsigned int On) { useBytes = On; }
  void SetSectionFiltering(unsigned int On) { sectionFiltering = On; }
  void SetSidScanning(unsigned int On) { sidScanning = On; }
  void SetDisabledFilters(unsigned int Index, int Number);
  void SetConfigDirectory(const char *directoryP);
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
