/*
 * setup.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: setup.h,v 1.2 2007/09/15 21:27:00 rahrenbe Exp $
 */

#ifndef __IPTV_SETUP_H
#define __IPTV_SETUP_H

#include <vdr/menuitems.h>

class cIptvPluginSetup : public cMenuSetupPage
{
private:
  int bufferSize;
  int bufferPrefill;
  virtual void Setup(void);

protected:
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Store(void);

public:
  cIptvPluginSetup();
};

#endif // __IPTV_SETUP_H
