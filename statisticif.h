/*
 * statisticif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statisticif.h,v 1.3 2007/10/07 10:13:45 ajhseppa Exp $
 */

#ifndef __IPTV_STATISTICIF_H
#define __IPTV_STATISTICIF_H

#include <vdr/tools.h>

class cIptvStatisticIf {
public:
  cIptvStatisticIf() {}
  virtual ~cIptvStatisticIf() {}
  virtual cString GetStatistic(uint64_t elapsed, const char* unit, const int unitdivider) = 0;

private:
    cIptvStatisticIf(const cIptvStatisticIf&);
    cIptvStatisticIf& operator=(const cIptvStatisticIf&);
};

#endif // __IPTV_STATISTICIF_H
