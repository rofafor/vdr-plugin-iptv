/*
 * config.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.c,v 1.3 2007/09/16 13:38:20 rahrenbe Exp $
 */

#include "common.h"
#include "config.h"

cIptvConfig IptvConfig;

cIptvConfig::cIptvConfig(void)
: tsBufferSize(8),
  tsBufferPrefillRatio(0),
  udpBufferSize(7),
  httpBufferSize(7),
  fileBufferSize(7)
{
}
