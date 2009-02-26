/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_CONFIG_H
#define __IPTV_CONFIG_H

#include <vdr/menuitems.h>
#include "common.h"

class cIptvConfig
{
private:
  unsigned int tsBufferSize;
  unsigned int tsBufferPrefillRatio;
  unsigned int extProtocolBasePort;
  unsigned int useBytes;
  unsigned int sectionFiltering;
  int disabledFilters[SECTION_FILTER_TABLE_SIZE];
  char configDirectory[255];

public:
  cIptvConfig();
  unsigned int GetTsBufferSize(void) const { return tsBufferSize; }
  unsigned int GetTsBufferPrefillRatio(void) const { return tsBufferPrefillRatio; }
  unsigned int GetExtProtocolBasePort(void) const { return extProtocolBasePort; }
  unsigned int GetUseBytes(void) const { return useBytes; }
  unsigned int GetSectionFiltering(void) const { return sectionFiltering; }
  const char *GetConfigDirectory(void) const { return configDirectory; }
  unsigned int GetDisabledFiltersCount(void) const;
  int GetDisabledFilters(unsigned int Index) const;
  void SetTsBufferSize(unsigned int Size) { tsBufferSize = Size; }
  void SetTsBufferPrefillRatio(unsigned int Ratio) { tsBufferPrefillRatio = Ratio; }
  void SetExtProtocolBasePort(unsigned int PortNumber) { extProtocolBasePort = PortNumber; }
  void SetUseBytes(unsigned int On) { useBytes = On; }
  void SetSectionFiltering(unsigned int On) { sectionFiltering = On; }
  void SetDisabledFilters(unsigned int Index, int Number);
  void SetConfigDirectory(const char *directoryP);
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
