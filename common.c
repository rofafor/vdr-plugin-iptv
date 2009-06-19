/*
 * common.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/tools.h>
#include "common.h"

uint16_t ts_pid(const uint8_t *buf)
{
  return (uint16_t)(((buf[1] & 0x1f) << 8) + buf[2]);
}

uint8_t payload(const uint8_t *tsp)
{
  if (!(tsp[3] & 0x10))        // no payload?
     return 0;

  if (tsp[3] & 0x20) { // adaptation field?
     if (tsp[4] > 183) // corrupted data?
        return 0;
     else
        return (uint8_t)((184 - 1) - tsp[4]);
     }

  return 184;
}

const char *id_pid(const u_short Pid)
{
  for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i) {
      if (Pid == section_filter_table[i].pid)
         return section_filter_table[i].tag;
      }
  return "---";
}

int select_single_desc(int descriptor, const int usecs, const bool selectWrite)
{
  // Wait for data
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = usecs;
  // Use select
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(descriptor, &fds);
  int retval = 0;
  if (selectWrite)
     retval = select(descriptor + 1, NULL, &fds, NULL, &tv);
  else
     retval = select(descriptor + 1, &fds, NULL, NULL, &tv);
  // Check if error
  ERROR_IF_RET(retval < 0, "select()", return retval);
  return retval;
}

const section_filter_table_type section_filter_table[SECTION_FILTER_TABLE_SIZE] =
{
  /* description              tag    pid   tid   mask */
  {trNOOP("PAT (0x00)"),      "PAT", 0x00, 0x00, 0xFF},
  {trNOOP("NIT (0x40)"),      "NIT", 0x10, 0x40, 0xFF},
  {trNOOP("SDT (0x42)"),      "SDT", 0x11, 0x42, 0xFF},
  {trNOOP("EIT (0x4E/0x4F)"), "EIT", 0x12, 0x4E, 0xFE},
  {trNOOP("EIT (0x5X)"),      "EIT", 0x12, 0x50, 0xF0},
  {trNOOP("EIT (0x6X)"),      "EIT", 0x12, 0x60, 0xF0},
  {trNOOP("TDT (0x70)"),      "TDT", 0x14, 0x70, 0xFF},
};

