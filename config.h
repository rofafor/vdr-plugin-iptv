/*
 * config.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.h,v 1.1 2007/09/15 15:38:38 rahrenbe Exp $
 */

#ifndef __IPTV_CONFIG_H
#define __IPTV_CONFIG_H

#include <vdr/menuitems.h>
#include "config.h"

class cIptvConfig
{
protected:
  unsigned int bufferSizeMB;
public:
  cIptvConfig();
  unsigned int GetBufferSizeMB(void) { return bufferSizeMB; }
  void SetBufferSizeMB(unsigned int Size) { bufferSizeMB = Size; }
};

extern cIptvConfig IptvConfig;

#endif // __IPTV_CONFIG_H
