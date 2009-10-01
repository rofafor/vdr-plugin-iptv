/*
 * sectionfilter.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_SECTIONFILTER_H
#define __IPTV_SECTIONFILTER_H

#include <vdr/device.h>

#include "common.h"
#include "statistics.h"

class cIptvSectionFilter : public cIptvSectionStatistics {
private:
  enum dmx_limits {
    DMX_MAX_FILTER_SIZE  = 18,
    DMX_MAX_SECTION_SIZE = 4096,
    DMX_MAX_SECFEED_SIZE = (DMX_MAX_SECTION_SIZE + TS_SIZE)
  };

  int pusi_seen;
  int feedcc;
  int doneq;

  uint8_t *secbuf;
  uint8_t secbuf_base[DMX_MAX_SECFEED_SIZE];
  uint16_t secbufp;
  uint16_t seclen;
  uint16_t tsfeedp;
  uint16_t pid;

  int devid;
  int id;
  int socket[2];

  uint8_t filter_value[DMX_MAX_FILTER_SIZE];
  uint8_t filter_mask[DMX_MAX_FILTER_SIZE];
  uint8_t filter_mode[DMX_MAX_FILTER_SIZE];

  uint8_t maskandmode[DMX_MAX_FILTER_SIZE];
  uint8_t maskandnotmode[DMX_MAX_FILTER_SIZE];

  inline uint16_t GetLength(const uint8_t *Data);
  void New(void);
  int Filter(void);
  inline int Feed(void);
  int CopyDump(const uint8_t *buf, uint8_t len);

public:
  // constructor & destructor
  cIptvSectionFilter(int Index, int DeviceIndex, uint16_t Pid,
                     uint8_t Tid, uint8_t Mask);
  virtual ~cIptvSectionFilter();
  void Process(const uint8_t* Data);
  int GetReadDesc(void);
  uint16_t GetPid(void) const { return pid; }
};

#endif // __IPTV_SECTIONFILTER_H
