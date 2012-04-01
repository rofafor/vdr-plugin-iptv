/*
 * iptvservice.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTVSERVICE_H
#define __IPTVSERVICE_H

#include <vdr/tools.h>

#define stIptv ('I' << 24)

struct IptvService_v1_0 {
  unsigned int cardIndex;
  cString protocol;
  cString bitrate;
  };

#endif //__IPTVSERVICE_H

