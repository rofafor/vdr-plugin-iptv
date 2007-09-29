/*
 * config.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.c,v 1.9 2007/09/29 16:21:05 rahrenbe Exp $
 */

#include "common.h"
#include "config.h"

cIptvConfig IptvConfig;

cIptvConfig::cIptvConfig(void)
: readBufferTsCount(48),
  tsBufferSize(2),
  tsBufferPrefillRatio(25),
  fileIdleTimeMs(5)
{
}
