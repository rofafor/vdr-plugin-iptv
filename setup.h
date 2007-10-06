/*
 * setup.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: setup.h,v 1.11 2007/10/06 20:57:53 rahrenbe Exp $
 */

#ifndef __IPTV_SETUP_H
#define __IPTV_SETUP_H

#include <vdr/menuitems.h>
#include "common.h"

class cIptvPluginSetup : public cMenuSetupPage
{
private:
  int tsBufferSize;
  int tsBufferPrefill;
  int sectionFiltering;
  int sidScanning;
  int numDisabledFilters;
  int disabledFilterIndexes[SECTION_FILTER_TABLE_SIZE];
  const char *disabledFilterNames[SECTION_FILTER_TABLE_SIZE];

  eOSState EditChannel(void);
  eOSState ShowInfo(void);
  virtual void Setup(void);
  void StoreFilters(const char *Name, int *Values);

protected:
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Store(void);

public:
  cIptvPluginSetup();
};

#endif // __IPTV_SETUP_H
