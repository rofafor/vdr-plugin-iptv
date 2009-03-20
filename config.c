/*
 * config.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "config.h"

cIptvConfig IptvConfig;

cIptvConfig::cIptvConfig(void)
: tsBufferSize(2),
  tsBufferPrefillRatio(0),
  extProtocolBasePort(4321),
  useBytes(1),
  sectionFiltering(1)
{
  for (unsigned int i = 0; i < ARRAY_SIZE(disabledFilters); ++i)
      disabledFilters[i] = -1;
  memset(configDirectory, '\0', sizeof(configDirectory));
}

unsigned int cIptvConfig::GetDisabledFiltersCount(void) const
{
  unsigned int n = 0;
  while ((n < ARRAY_SIZE(disabledFilters) && (disabledFilters[n] != -1)))
        n++;
  return n;
}

int cIptvConfig::GetDisabledFilters(unsigned int Index) const
{
  return (Index < ARRAY_SIZE(disabledFilters)) ? disabledFilters[Index] : -1;
}

void cIptvConfig::SetDisabledFilters(unsigned int Index, int Number)
{
  if (Index < ARRAY_SIZE(disabledFilters))
     disabledFilters[Index] = Number;
}

void cIptvConfig::SetConfigDirectory(const char *directoryP)
{
  debug("cIptvConfig::SetConfigDirectory(%s)", directoryP);
  strn0cpy(configDirectory, directoryP, sizeof(configDirectory));
}
