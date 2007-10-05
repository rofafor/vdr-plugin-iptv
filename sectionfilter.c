/*
 * sectionfilter.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: sectionfilter.c,v 1.7 2007/10/05 19:00:44 ajhseppa Exp $
 */

#include "sectionfilter.h"
#include "statistics.h"

#define IPTV_FILTER_FILENAME "/tmp/vdr-iptv%d.filter%d"

uint16_t ts_pid(const uint8_t *buf)
{
  return ((buf[1] & 0x1f) << 8) + buf[2];
}

uint8_t payload(const uint8_t *tsp)
{
  if (!(tsp[3] & 0x10))	// no payload?
     return 0;

  if (tsp[3] & 0x20) {	// adaptation field?
     if (tsp[4] > 183)	// corrupted data?
        return 0;
     else
        return 184 - 1 - tsp[4];
     }

  return 184;
}

cIptvSectionFilter::cIptvSectionFilter(int Index, int devInd,
				       u_short Pid, u_char Tid, u_char Mask)
: pusi_seen(0),
  feedcc(0),
  doneq(0),
  secbuf(NULL),
  secbufp(0),
  seclen(0),
  tsfeedp(0),
  crc_val(0),
  pid(Pid),
  id(Index)
{
  //debug("cIptvSectionFilter::cIptvSectionFilter(%d)\n", Index);
  memset(secbuf_base, '\0', sizeof(secbuf_base));
  memset(filter_value, '\0', sizeof(filter_value));
  memset(filter_mask, '\0', sizeof(filter_mask));
  memset(filter_mode, '\0', sizeof(filter_mode));
  memset(maskandmode, '\0', sizeof(maskandmode));
  memset(maskandnotmode, '\0', sizeof(maskandnotmode));
  memset(pipeName, '\0', sizeof(pipeName));

  SetPipeName(devInd);

  filter_value[0] = Tid;
  filter_mask[0] = Mask;

  // Invert the filter
  for (int i = 0; i < DVB_DEMUX_MASK_MAX; ++i) {
      filter_value[i] ^= 0xff;
  }

  uint8_t mask, mode, local_doneq = 0;
  for (int i = 0; i < DVB_DEMUX_MASK_MAX; i++) {
    mode = filter_mode[i];
    mask = filter_mask[i];
    maskandmode[i] = mask & mode;
    local_doneq |= maskandnotmode[i] = mask & ~mode;
  }
  doneq = local_doneq ? 1 : 0;

  struct stat sb;
  stat(pipeName, &sb);
  if (S_ISFIFO(sb.st_mode))
     unlink(pipeName);
  int err = mknod(pipeName, 0644 | S_IFIFO, 0);
  if (err < 0) {
     char tmp[64];
     error("ERROR: mknod(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     return;
     }

  // Create descriptors
  fifoDescriptor = open(pipeName, O_RDWR | O_NONBLOCK);
  readDescriptor = open(pipeName, O_RDONLY | O_NONBLOCK);
}

cIptvSectionFilter::~cIptvSectionFilter()
{
  //debug("cIptvSectionFilter::~cIptvSectionfilter(%d)\n", id);
  close(fifoDescriptor);
  close(readDescriptor);
  unlink(pipeName);
  memset(pipeName, '\0', sizeof(pipeName));
  fifoDescriptor = -1;
  readDescriptor = -1;
}

void cIptvSectionFilter::SetPipeName(int deviceIndex)
{
  snprintf(pipeName, sizeof(pipeName), IPTV_FILTER_FILENAME, deviceIndex, id);
}

int cIptvSectionFilter::GetReadDesc()
{
  return readDescriptor;
}

inline uint16_t cIptvSectionFilter::section_length(const uint8_t *buf)
{
  return 3 + ((buf[1] & 0x0f) << 8) + buf[2];
}

int cIptvSectionFilter::dvb_dmxdev_section_callback(const uint8_t *buffer1, size_t buffer1_len,
                                                    const uint8_t *buffer2, size_t buffer2_len,
                                                    enum dmx_success success)
{
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(fifoDescriptor, &rfds);
  int retval = select(fifoDescriptor + 1, &rfds, NULL, NULL, &tv);

  // Check if error
  if (retval < 0) {
     char tmp[64];
     error("ERROR: select(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     }
  // There is no data in the fifo, more can be written
  else if (!retval) {
#ifdef DEBUG_PRINTF
     printf("id = %d, pid %d would now write %d data to buffer\n", id, pid, buffer1_len);
     for (unsigned int i = 0; i < buffer1_len; ++i)
         printf("0x%X ", buffer1[i]);
     printf("\n");
#endif
     retval = write(fifoDescriptor, buffer1, buffer1_len);
     if (retval < 0) {
        char tmp[64];
        error("ERROR: write(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        }
     // Increment statistics counters
     filteredData += retval;
     ++numberOfCalls;
     }
#ifdef DEBUG_PRINTF
  else if (retval)
     printf("id %d pid %d data is already present\n", id, pid);
#endif
  return 0;
}



void cIptvSectionFilter::dvb_dmx_swfilter_section_new()
{
#ifdef DVB_DEMUX_SECTION_LOSS_LOG
  if (secbufp < tsfeedp) {
     int i, n = tsfeedp - secbufp;
     /*
      * Section padding is done with 0xff bytes entirely.
      * Due to speed reasons, we won't check all of them
      * but just first and last.
      */
     if (secbuf[0] != 0xff || secbuf[n - 1] != 0xff) {
        printf("dvb_demux.c section ts padding loss: %d/%d\n",
               n, tsfeedp);
        printf("dvb_demux.c pad data:");
        for (i = 0; i < n; i++)
	    printf(" %02x", secbuf[i]);
        printf("\n");
        }
     }
#endif
  tsfeedp = secbufp = seclen = 0;
  secbuf = secbuf_base;
}

int cIptvSectionFilter::dvb_dmx_swfilter_sectionfilter()
{
  uint8_t neq = 0;
  int i;

  for (i = 0; i < DVB_DEMUX_MASK_MAX; i++) {
      uint8_t local_xor = filter_value[i] ^ secbuf[i];
      if (maskandmode[i] & local_xor) {
#ifdef DEBUG_PRINTF
         printf("maskandmode discard\n");
#endif
         return 0;
         }
      neq |= maskandnotmode[i] & local_xor;
      }

  if (doneq && !neq) {
#ifdef DEBUG_PRINTF
     printf("doneq discard, doneq = 0x%X, neq = 0x%X\n", doneq, !neq);
#endif
     return 0;
     }
  return dvb_dmxdev_section_callback(secbuf, seclen, NULL, 0, DMX_OK);
}

inline int cIptvSectionFilter::dvb_dmx_swfilter_section_feed()
{
  if (dvb_dmx_swfilter_sectionfilter() < 0)
     return -1;
  seclen = 0;
  return 0;
}

int cIptvSectionFilter::dvb_dmx_swfilter_section_copy_dump(const uint8_t *buf, uint8_t len)
{
  uint16_t limit, seclen_local, n;

  if (tsfeedp >= DMX_MAX_SECFEED_SIZE)
     return 0;

  if (tsfeedp + len > DMX_MAX_SECFEED_SIZE) {
#ifdef DVB_DEMUX_SECTION_LOSS_LOG
     printf("dvb_demux.c section buffer full loss: %d/%d\n",
            tsfeedp + len - DMX_MAX_SECFEED_SIZE,
            DMX_MAX_SECFEED_SIZE);
#endif
     len = DMX_MAX_SECFEED_SIZE - tsfeedp;
     }

  if (len <= 0)
     return 0;

  memcpy(secbuf_base + tsfeedp, buf, len);
  tsfeedp += len;

  /*
   * Dump all the sections we can find in the data (Emard)
   */
  limit = tsfeedp;
  if (limit > DMX_MAX_SECFEED_SIZE)
     return -1;	/* internal error should never happen */

  /* to be sure always set secbuf */
  secbuf = secbuf_base + secbufp;

  for (n = 0; secbufp + 2 < limit; n++) {
      seclen_local = section_length(secbuf);
      if (seclen_local <= 0 || seclen_local > DMX_MAX_SECTION_SIZE ||
         seclen_local + secbufp > limit)
         return 0;
#ifdef DEBUG_PRINTF
      printf("Non-mismatching seclen! %d, limit = %d\n", seclen_local, limit);
#endif
      seclen = seclen_local;
      crc_val = ~0;
      /* dump [secbuf .. secbuf+seclen) */
      if (pusi_seen)
         dvb_dmx_swfilter_section_feed();
#ifdef DVB_DEMUX_SECTION_LOSS_LOG
      else
         printf("dvb_demux.c pusi not seen, discarding section data\n");
#endif
      secbufp += seclen_local;	/* secbufp and secbuf moving together is */
      secbuf += seclen_local;	/* redundant but saves pointer arithmetic */
      }
  return 0;
}

void cIptvSectionFilter::ProcessData(const uint8_t* buf)
{
  if (buf[0] != 0x47) {
     error("Not TS packet: 0x%X\n", buf[0]);
     return;
     }
  
  // Stop if not the PID this filter is looking for
  if (ts_pid(buf) != pid)
     return;

  uint8_t count = payload(buf);

  if (count == 0)		/* count == 0 if no payload or out of range */
     return;

  uint8_t p = 188 - count;	/* payload start */

  uint8_t cc = buf[3] & 0x0f;
  int ccok = ((feedcc + 1) & 0x0f) == cc;
  feedcc = cc;  

  int dc_i = 0;
  if (buf[3] & 0x20) {
     /* adaption field present, check for discontinuity_indicator */
     if ((buf[4] > 0) && (buf[5] & 0x80))
        dc_i = 1;
     }

  if (!ccok || dc_i) {
#ifdef DVB_DEMUX_SECTION_LOSS_LOG
     printf("dvb_demux.c discontinuity detected %d bytes lost\n",
            count);
     /*
      * those bytes under sume circumstances will again be reported
      * in the following dvb_dmx_swfilter_section_new
      */
#endif
     /*
      * Discontinuity detected. Reset pusi_seen = 0 to
      * stop feeding of suspicious data until next PUSI=1 arrives
      */
     pusi_seen = 0;
     dvb_dmx_swfilter_section_new();
     }

  if (buf[1] & 0x40) {
     /* PUSI=1 (is set), section boundary is here */
     if (count > 1 && buf[p] < count) {
#ifdef DEBUG_PRINTF
        printf("Valid section\n");
#endif
        const uint8_t *before = &buf[p + 1];
        uint8_t before_len = buf[p];
        const uint8_t *after = &before[before_len];
        uint8_t after_len = count - 1 - before_len;

        dvb_dmx_swfilter_section_copy_dump(before, before_len);
        /* before start of new section, set pusi_seen = 1 */
        pusi_seen = 1;
        dvb_dmx_swfilter_section_new();
        dvb_dmx_swfilter_section_copy_dump(after, after_len);
        }
#ifdef DVB_DEMUX_SECTION_LOSS_LOG
     else if (count > 0)
        printf("dvb_demux.c PUSI=1 but %d bytes lost\n", count);
#endif
     }
  else {
     /* PUSI=0 (is not set), no section boundary */
     dvb_dmx_swfilter_section_copy_dump(&buf[p], count);
     }
}
