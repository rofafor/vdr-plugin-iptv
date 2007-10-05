/*
 * sectionfilter.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: sectionfilter.h,v 1.4 2007/10/05 20:01:24 ajhseppa Exp $
 */

#ifndef __IPTV_SECTIONFILTER_H
#define __IPTV_SECTIONFILTER_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <libgen.h>
#include <stdint.h>
#include <sys/param.h>

#include "common.h"
#include "statistics.h"

#ifndef DMX_MAX_FILTER_SIZE
#define DMX_MAX_FILTER_SIZE 18
#endif

#ifndef DMX_MAX_SECTION_SIZE
#define DMX_MAX_SECTION_SIZE 4096
#endif
#ifndef DMX_MAX_SECFEED_SIZE
#define DMX_MAX_SECFEED_SIZE (DMX_MAX_SECTION_SIZE + 188)
#endif

#define DVB_DEMUX_MASK_MAX 18

class cIptvSectionFilter : public cIptvSectionStatistics {
private:
  enum dmx_success {
    DMX_OK = 0, /* Received Ok */
    DMX_LENGTH_ERROR, /* Incorrect length */
    DMX_OVERRUN_ERROR, /* Receiver ring buffer overrun */
    DMX_CRC_ERROR, /* Incorrect CRC */
    DMX_FRAME_ERROR, /* Frame alignment error */
    DMX_FIFO_ERROR, /* Receiver FIFO overrun */
    DMX_MISSED_ERROR /* Receiver missed packet */
  };

  int fifoDescriptor;
  int readDescriptor;

  int pusi_seen;
  int feedcc;
  int doneq;

  uint8_t *secbuf;
  uint8_t secbuf_base[DMX_MAX_SECFEED_SIZE];
  uint16_t secbufp;
  uint16_t seclen;
  uint16_t tsfeedp;
  uint32_t crc_val;

  uint16_t pid;
  int id;

  uint8_t filter_value[DMX_MAX_FILTER_SIZE];
  uint8_t filter_mask[DMX_MAX_FILTER_SIZE];
  uint8_t filter_mode[DMX_MAX_FILTER_SIZE];
   
  uint8_t maskandmode[DMX_MAX_FILTER_SIZE];
  uint8_t maskandnotmode[DMX_MAX_FILTER_SIZE];

  char pipeName[128];

  inline uint16_t section_length(const uint8_t *buf);

  int dvb_dmxdev_section_callback(const uint8_t *buffer1,
                                  size_t buffer1_len,
                                  const uint8_t *buffer2,
                                  size_t buffer2_len,
                                  enum dmx_success success);

  void dvb_dmx_swfilter_section_new();

  int dvb_dmx_swfilter_sectionfilter();

  inline int dvb_dmx_swfilter_section_feed();

  int dvb_dmx_swfilter_section_copy_dump(const uint8_t *buf,
                                         uint8_t len);

  int GetFifoDesc();
  void ClearPipeName();
  void SetPipeName(int deviceIndex);

public:
  // constructor & destructor
  cIptvSectionFilter(int Index, int devInd, u_short Pid, u_char Tid,
                     u_char Mask);

  virtual ~cIptvSectionFilter();

  void ProcessData(const uint8_t* buf);

  int GetReadDesc();
};

#endif // __IPTV_SECTIONFILTER_H
