/*
 * common.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: common.h,v 1.18 2007/10/28 16:22:44 rahrenbe Exp $
 */

#ifndef __IPTV_COMMON_H
#define __IPTV_COMMON_H

#include <vdr/tools.h>
#include <vdr/config.h>
#include <vdr/i18n.h>

#ifdef DEBUG
#define debug(x...) dsyslog("IPTV: " x);
#define error(x...) esyslog("IPTV: " x);
#else
#define debug(x...) ;
#define error(x...) esyslog("IPTV: " x);
#endif

#ifndef trNOOP
#define trNOOP(s) (s)
#endif

#ifndef trVDR
#define trVDR(s) tr(s)
#endif

#define IPTV_DEVICE_INFO_ALL            0
#define IPTV_DEVICE_INFO_GENERAL        1
#define IPTV_DEVICE_INFO_PIDS           2
#define IPTV_DEVICE_INFO_FILTERS        3

#define IPTV_STATS_ACTIVE_PIDS_COUNT    10
#define IPTV_STATS_ACTIVE_FILTERS_COUNT 10

#define SECTION_FILTER_TABLE_SIZE       7

#define ERROR_IF_FUNC(exp, errstr, func, ret) \
  do {                                                                     \
     if (exp) {                                                            \
        char tmp[64];                                                      \
        error("ERROR: "errstr": %s", strerror_r(errno, tmp, sizeof(tmp))); \
        func;                                                              \
        ret;                                                               \
        }                                                                  \
  } while (0)


#define ERROR_IF_RET(exp, errstr, ret) ERROR_IF_FUNC(exp, errstr, ,ret);

#define ERROR_IF(exp, errstr) ERROR_IF_FUNC(exp, errstr, , );

#define DELETE_POINTER(ptr)       \
  do {                            \
     if (ptr) {                   \
        typeof(*ptr) *tmp = ptr;  \
        ptr = NULL;               \
        delete(tmp);              \
        }                         \
  } while (0)

uint16_t ts_pid(const uint8_t *buf);
uint8_t payload(const uint8_t *tsp);
const char *id_pid(const u_short Pid);
int select_single_desc(int descriptor, const int usecs, const bool selectWrite);

struct section_filter_table_type {
  const char *description;
  const char *tag;
  u_short pid;
  u_char tid;
  u_char mask;
};

extern const section_filter_table_type section_filter_table[SECTION_FILTER_TABLE_SIZE];

#endif // __IPTV_COMMON_H

