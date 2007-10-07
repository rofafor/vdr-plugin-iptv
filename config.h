/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.11 2007/10/07 19:06:33 ajhseppa Exp $
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
  unsigned int statsInKilos;
  unsigned int statsInBytes;
  int disabledFilters[SECTION_FILTER_TABLE_SIZE];

public:
  cIptvConfig();
  unsigned int GetReadBufferTsCount(void) { return readBufferTsCount; }
  unsigned int GetTsBufferSize(void) { return tsBufferSize; }
  unsigned int GetTsBufferPrefillRatio(void) { return tsBufferPrefillRatio; }
  unsigned int GetSectionFiltering(void) { return sectionFiltering; }
  unsigned int GetSidScanning(void) { return sidScanning; }
  unsigned int GetStatsInBytes(void) { return statsInBytes; }
  unsigned int GetStatsInKilos(void) { return statsInKilos; }
  unsigned int GetDisabledFiltersCount(void);
  int GetDisabledFilters(unsigned int Index);
  void SetTsBufferSize(unsigned int Size) { tsBufferSize = Size; }
  void SetTsBufferPrefillRatio(unsigned int Ratio) { tsBufferPrefillRatio = Ratio; }
  void SetSectionFiltering(unsigned int On) { sectionFiltering = On; }
  void SetSidScanning(unsigned int On) { sidScanning = On; }
  void SetStatsInBytes(unsigned int On) { statsInBytes = On; }
  void SetStatsInKilos(unsigned int On) { statsInKilos = On; }
  void SetDisabledFilters(unsigned int Index, int Number);
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
