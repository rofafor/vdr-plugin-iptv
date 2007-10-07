/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.12 2007/10/07 20:08:44 rahrenbe Exp $
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
  unsigned int statsUnit;
  unsigned int sectionFiltering;
  unsigned int sidScanning;
  int disabledFilters[SECTION_FILTER_TABLE_SIZE];

public:
  cIptvConfig();
  unsigned int GetReadBufferTsCount(void) { return readBufferTsCount; }
  unsigned int GetTsBufferSize(void) { return tsBufferSize; }
  unsigned int GetTsBufferPrefillRatio(void) { return tsBufferPrefillRatio; }
  unsigned int GetStatsUnit(void) { return statsUnit; }
  unsigned int IsStatsUnitInBytes(void) { return ((statsUnit == IPTV_STATS_UNIT_IN_BYTES) ||
                                                  (statsUnit == IPTV_STATS_UNIT_IN_KBYTES)); }
  unsigned int IsStatsUnitInKilos(void) { return ((statsUnit == IPTV_STATS_UNIT_IN_KBYTES) ||
                                                  (statsUnit == IPTV_STATS_UNIT_IN_KBITS)); }
  unsigned int GetSectionFiltering(void) { return sectionFiltering; }
  unsigned int GetSidScanning(void) { return sidScanning; }
  unsigned int GetDisabledFiltersCount(void);
  int GetDisabledFilters(unsigned int Index);
  void SetTsBufferSize(unsigned int Size) { tsBufferSize = Size; }
  void SetTsBufferPrefillRatio(unsigned int Ratio) { tsBufferPrefillRatio = Ratio; }
  void SetStatsUnit(unsigned int Unit) { statsUnit = Unit; }
  void SetSectionFiltering(unsigned int On) { sectionFiltering = On; }
  void SetSidScanning(unsigned int On) { sidScanning = On; }
  void SetDisabledFilters(unsigned int Index, int Number);
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
