/*
 * config.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.c,v 1.4 2007/09/20 21:45:51 rahrenbe Exp $
 */

#include "common.h"
#include "config.h"

cIptvConfig IptvConfig;

cIptvConfig::cIptvConfig(void)
: tsBufferSize(8),
  tsBufferPrefillRatio(0),
  udpBufferSize(7),
  httpBufferSize(7),
  fileBufferSize(20),
  maxBufferSize(40) // must be bigger than protocol buffer sizes!
{
}
