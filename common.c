/*
 * common.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: common.c,v 1.1 2007/10/05 20:01:24 ajhseppa Exp $
 */


#include <vdr/tools.h>

uint16_t ts_pid(const uint8_t *buf)
{
  return ((buf[1] & 0x1f) << 8) + buf[2];
}

uint8_t payload(const uint8_t *tsp)
{
  if (!(tsp[3] & 0x10))        // no payload?
     return 0;

  if (tsp[3] & 0x20) { // adaptation field?
     if (tsp[4] > 183) // corrupted data?
        return 0;
     else
        return 184 - 1 - tsp[4];
     }

  return 184;
}
