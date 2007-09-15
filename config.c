/*
 * config.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.c,v 1.2 2007/09/15 21:27:00 rahrenbe Exp $
 */

#include "common.h"
#include "config.h"

cIptvConfig IptvConfig;

cIptvConfig::cIptvConfig(void)
: bufferSizeMB(8),
  bufferPrefillRatio(0)
{
}
