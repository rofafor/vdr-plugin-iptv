/*
 * setup.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_SETUP_H
#define __IPTV_SETUP_H

#include <vdr/menuitems.h>
#include <vdr/sourceparams.h>
#include "common.h"

class cIptvPluginSetup : public cMenuSetupPage
{
private:
  int protocolBasePortM;
  int sectionFilteringM;
  int numDisabledFiltersM;
  int disabledFilterIndexesM[SECTION_FILTER_TABLE_SIZE];
  const char *disabledFilterNamesM[SECTION_FILTER_TABLE_SIZE];
  cVector<const char*> helpM;

  eOSState ShowInfo(void);
  void Setup(void);
  void StoreFilters(const char *nameP, int *valuesP);

protected:
  virtual eOSState ProcessKey(eKeys keyP);
  virtual void Store(void);

public:
  cIptvPluginSetup();
};

#endif // __IPTV_SETUP_H
