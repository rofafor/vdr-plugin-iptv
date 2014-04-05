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
  unsigned int protocolBasePortM;
  unsigned int useBytesM;
  unsigned int sectionFilteringM;
  int disabledFiltersM[SECTION_FILTER_TABLE_SIZE];
  char configDirectoryM[PATH_MAX];
  char resourceDirectoryM[PATH_MAX];

public:
  cIptvConfig();
  unsigned int GetProtocolBasePort(void) const { return protocolBasePortM; }
  unsigned int GetUseBytes(void) const { return useBytesM; }
  unsigned int GetSectionFiltering(void) const { return sectionFilteringM; }
  const char *GetConfigDirectory(void) const { return configDirectoryM; }
  const char *GetResourceDirectory(void) const { return resourceDirectoryM; }
  unsigned int GetDisabledFiltersCount(void) const;
  int GetDisabledFilters(unsigned int indexP) const;
  void SetProtocolBasePort(unsigned int portNumberP) { protocolBasePortM = portNumberP; }
  void SetUseBytes(unsigned int onOffP) { useBytesM = onOffP; }
  void SetSectionFiltering(unsigned int onOffP) { sectionFilteringM = onOffP; }
  void SetDisabledFilters(unsigned int indexP, int numberP);
  void SetConfigDirectory(const char *directoryP);
  void SetResourceDirectory(const char *directoryP);
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
