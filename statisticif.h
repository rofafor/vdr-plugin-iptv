/*
 * statisticif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: statisticif.h,v 1.1 2007/10/05 19:00:44 ajhseppa Exp $
 */

#ifndef __IPTV_STATISTICIF_H
#define __IPTV_STATISTICIF_H

class cIptvStatisticIf {
public:
  cIptvStatisticIf() {}
  virtual ~cIptvStatisticIf() {}
  virtual char* GetStatistic() = 0;

private:
    cIptvStatisticIf(const cIptvStatisticIf&);
    cIptvStatisticIf& operator=(const cIptvStatisticIf&);
};

#endif // __IPTV_STATISTICIF_H
