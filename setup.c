/*
 * setup.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: setup.c,v 1.2 2007/09/15 21:27:00 rahrenbe Exp $
 */

#include "common.h"
#include "config.h"
#include "setup.h"

cIptvPluginSetup::cIptvPluginSetup(void)
{
  bufferSize = IptvConfig.GetBufferSizeMB();
  bufferPrefill = IptvConfig.GetBufferPrefillRatio();
  Setup();
}

void cIptvPluginSetup::Setup(void)
{
  int current = Current();
  Clear();
  Add(new cMenuEditIntItem(tr("Buffer size [MB]"), &bufferSize, 0, 16));
  Add(new cMenuEditIntItem(tr("Buffer prefill ratio [%]"), &bufferPrefill, 0, 40));
  SetCurrent(Get(current));
  Display();
}

eOSState cIptvPluginSetup::ProcessKey(eKeys Key)
{
  eOSState state = cMenuSetupPage::ProcessKey(Key);
  return state;
}

void cIptvPluginSetup::Store(void)
{
  SetupStore("BufferSize", bufferSize);
  SetupStore("BufferPrefill", bufferPrefill);
  IptvConfig.SetBufferSizeMB(bufferSize);
  IptvConfig.SetBufferPrefillRatio(bufferPrefill);
}