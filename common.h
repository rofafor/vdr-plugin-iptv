/*
 * common.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: common.h,v 1.6 2007/10/07 22:54:09 rahrenbe Exp $
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

#define IPTV_STATS_UNIT_IN_BYTES     0
#define IPTV_STATS_UNIT_IN_KBYTES    1
#define IPTV_STATS_UNIT_IN_BITS      2
#define IPTV_STATS_UNIT_IN_KBITS     3
#define IPTV_STATS_UNIT_COUNT        4

#define IPTV_STATS_ACTIVE_PIDS_COUNT 10

#define SECTION_FILTER_TABLE_SIZE    7

typedef struct _section_filter_table_type {
  const char *description;
  u_short pid;
  u_char tid;
  u_char mask;
} section_filter_table_type;

extern const section_filter_table_type section_filter_table[SECTION_FILTER_TABLE_SIZE];

#endif // __IPTV_COMMON_H

