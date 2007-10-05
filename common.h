/*
 * common.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: common.h,v 1.3 2007/10/05 20:01:24 ajhseppa Exp $
 */

#ifndef __IPTV_COMMON_H
#define __IPTV_COMMON_H

#include <vdr/tools.h>

uint16_t ts_pid(const uint8_t *buf);
uint8_t payload(const uint8_t *tsp);

#ifdef DEBUG
#define debug(x...) dsyslog("IPTV: " x);
#define error(x...) esyslog("IPTV: " x);
#else
#define debug(x...) ;
#define error(x...) esyslog("IPTV: " x);
#endif

#endif // __IPTV_COMMON_H

