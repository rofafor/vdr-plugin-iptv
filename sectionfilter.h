/*
 * sectionfilter.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_SECTIONFILTER_H
#define __IPTV_SECTIONFILTER_H

#ifdef __FreeBSD__
#include <sys/socket.h>
#endif // __FreeBSD__
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

  int pusiSeenM;
  int feedCcM;
  int doneqM;

  uint8_t *secBufM;
  uint8_t secBufBaseM[DMX_MAX_SECFEED_SIZE];
  uint16_t secBufpM;
  uint16_t secLenM;
  uint16_t tsFeedpM;
  uint16_t pidM;

  int devIdM;

  uint8_t filterValueM[DMX_MAX_FILTER_SIZE];
  uint8_t filterMaskM[DMX_MAX_FILTER_SIZE];
  uint8_t filterModeM[DMX_MAX_FILTER_SIZE];

  uint8_t maskAndModeM[DMX_MAX_FILTER_SIZE];
  uint8_t maskAndNotModeM[DMX_MAX_FILTER_SIZE];

  cRingBufferLinear *ringbufferM;

  inline uint16_t GetLength(const uint8_t *dataP);
  void New(void);
  int Filter(void);
  inline int Feed(void);
  int CopyDump(const uint8_t *bufP, uint8_t lenP);

public:
  // constructor & destructor
  cIptvSectionFilter(int deviceIndexP, uint16_t pidP, uint8_t tidP, uint8_t maskP);
  virtual ~cIptvSectionFilter();
  void Process(const uint8_t* dataP);
  int Read(void *bufferP, size_t lengthP);
  uint16_t GetPid(void) const { return pidM; }
};

#endif // __IPTV_SECTIONFILTER_H
