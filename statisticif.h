/*
 * statisticif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statisticif.h,v 1.4 2007/10/07 19:06:33 ajhseppa Exp $
 */

#ifndef __IPTV_STATISTICIF_H
#define __IPTV_STATISTICIF_H

#include <vdr/tools.h>

class cIptvStatisticIf {
public:
  cIptvStatisticIf() {}
  virtual ~cIptvStatisticIf() {}
  virtual cString GetStatistic() = 0;

private:
    cIptvStatisticIf(const cIptvStatisticIf&);
    cIptvStatisticIf& operator=(const cIptvStatisticIf&);
};

#endif // __IPTV_STATISTICIF_H
