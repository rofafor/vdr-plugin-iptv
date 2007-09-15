/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.2 2007/09/15 21:27:00 rahrenbe Exp $
 */

#ifndef __IPTV_CONFIG_H
#define __IPTV_CONFIG_H

#include <vdr/menuitems.h>
#include "config.h"

class cIptvConfig
{
protected:
  unsigned int bufferSizeMB;
  unsigned int bufferPrefillRatio;
public:
  cIptvConfig();
  unsigned int GetBufferSizeMB(void) { return bufferSizeMB; }
  unsigned int GetBufferPrefillRatio(void) { return bufferPrefillRatio; }
  void SetBufferSizeMB(unsigned int Size) { bufferSizeMB = Size; }
  void SetBufferPrefillRatio(unsigned int Ratio) { bufferPrefillRatio = Ratio; }
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
