/*
 * setup.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: setup.c,v 1.5 2007/09/16 13:49:35 rahrenbe Exp $
 */

#include "common.h"
#include "config.h"
#include "setup.h"

#ifndef trVDR
#define trVDR(s) tr(s)
#endif
	
cIptvPluginSetup::cIptvPluginSetup(void)
{
  tsBufferSize = IptvConfig.GetTsBufferSize();
  tsBufferPrefill = IptvConfig.GetTsBufferPrefillRatio();
  udpBufferSize = IptvConfig.GetUdpBufferSize();
  httpBufferSize = IptvConfig.GetHttpBufferSize();
  fileBufferSize = IptvConfig.GetFileBufferSize();
  Setup();
  SetHelp(trVDR("Channels"), NULL, NULL, NULL);
}

void cIptvPluginSetup::Setup(void)
{
  int current = Current();
  Clear();
  Add(new cMenuEditIntItem(tr("TS buffer size [MB]"), &tsBufferSize, 2, 16));
  Add(new cMenuEditIntItem(tr("TS buffer prefill ratio [%]"), &tsBufferPrefill, 0, 40));
  Add(new cMenuEditIntItem(tr("UDP buffer size [packets]"), &udpBufferSize, 1, 14));
  Add(new cMenuEditIntItem(tr("HTTP buffer size [packets]"), &httpBufferSize, 1, 14));
  Add(new cMenuEditIntItem(tr("FILE buffer size [packets]"), &fileBufferSize, 1, 14));
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
  // Store values into setup.conf
  SetupStore("TsBufferSize", tsBufferSize);
  SetupStore("TsBufferPrefill", tsBufferPrefill);
  SetupStore("UdpBufferSize", udpBufferSize);
  SetupStore("HttpBufferSize", httpBufferSize);
  SetupStore("FileBufferSize", fileBufferSize);
  // Update global config
  IptvConfig.SetTsBufferSize(tsBufferSize);
  IptvConfig.SetTsBufferPrefillRatio(tsBufferPrefill);
  IptvConfig.SetUdpBufferSize(udpBufferSize);
  IptvConfig.SetHttpBufferSize(httpBufferSize);
  IptvConfig.SetFileBufferSize(fileBufferSize);
}
