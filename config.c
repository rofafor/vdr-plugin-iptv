/*
 * config.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.c,v 1.14 2007/10/07 20:08:44 rahrenbe Exp $
 */

#include "config.h"

cIptvConfig IptvConfig;

cIptvConfig::cIptvConfig(void)
: readBufferTsCount(48),
  tsBufferSize(2),
  tsBufferPrefillRatio(0),
  statsUnit(IPTV_STATS_UNIT_IN_KBYTES),
  sectionFiltering(1),
  sidScanning(1)
{
  for (unsigned int i = 0; i < sizeof(disabledFilters); ++i)
      disabledFilters[i] = -1;
}

unsigned int cIptvConfig::GetDisabledFiltersCount(void)
{
  unsigned int n = 0;
  while ((disabledFilters[n] != -1) && (n < sizeof(disabledFilters)))
    n++;
  return n;
}

int cIptvConfig::GetDisabledFilters(unsigned int Index)
{
  return (Index < sizeof(disabledFilters)) ? disabledFilters[Index] : -1;
}

void cIptvConfig::SetDisabledFilters(unsigned int Index, int Number)
{
  if (Index < sizeof(disabledFilters))
     disabledFilters[Index] = Number;
}
