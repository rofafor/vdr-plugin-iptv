/*
 * config.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: config.c,v 1.6 2007/09/27 22:30:50 rahrenbe Exp $
 */

#include "common.h"
#include "config.h"

cIptvConfig IptvConfig;

cIptvConfig::cIptvConfig(void)
: tsBufferSize(2),
  tsBufferPrefillRatio(0),
  udpBufferSize(20),
  rtpBufferSize(20),
  httpBufferSize(20),
  fileBufferSize(20),
  maxBufferSize(40) // must be bigger than protocol buffer sizes!
{
}
