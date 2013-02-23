/*
 * config.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "config.h"

cIptvConfig IptvConfig;

cIptvConfig::cIptvConfig(void)
: tsBufferSizeM(2),
  tsBufferPrefillRatioM(0),
  extProtocolBasePortM(4321),
  useBytesM(1),
  sectionFilteringM(1)
{
  for (unsigned int i = 0; i < ARRAY_SIZE(disabledFiltersM); ++i)
      disabledFiltersM[i] = -1;
  memset(configDirectoryM, 0, sizeof(configDirectoryM));
}

unsigned int cIptvConfig::GetDisabledFiltersCount(void) const
{
  unsigned int n = 0;
  while ((n < ARRAY_SIZE(disabledFiltersM) && (disabledFiltersM[n] != -1)))
        n++;
  return n;
}

int cIptvConfig::GetDisabledFilters(unsigned int indexP) const
{
  return (indexP < ARRAY_SIZE(disabledFiltersM)) ? disabledFiltersM[indexP] : -1;
}

void cIptvConfig::SetDisabledFilters(unsigned int indexP, int numberP)
{
  if (indexP < ARRAY_SIZE(disabledFiltersM))
     disabledFiltersM[indexP] = numberP;
}

void cIptvConfig::SetConfigDirectory(const char *directoryP)
{
  debug("cIptvConfig::%s(%s)", __FUNCTION__, directoryP);
  ERROR_IF(!realpath(directoryP, configDirectoryM), "Cannot canonicalize configuration directory");
}
