/*
 * sectionfilter.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "config.h"
#include "sectionfilter.h"

cIptvSectionFilter::cIptvSectionFilter(int deviceIndexP, uint16_t pidP, uint8_t tidP, uint8_t maskP)
: pusiSeenM(0),
  feedCcM(0),
  doneqM(0),
  secBufM(NULL),
  secBufpM(0),
  secLenM(0),
  tsFeedpM(0),
  pidM(pidP),
  devIdM(deviceIndexP)
{
  //debug("cIptvSectionFilter::cIptvSectionFilter(%d, %d)\n", devIdM, pidM);
  int i;

  memset(secBufBaseM,     0, sizeof(secBufBaseM));
  memset(filterValueM,    0, sizeof(filterValueM));
  memset(filterMaskM,     0, sizeof(filterMaskM));
  memset(filterModeM,     0, sizeof(filterModeM));
  memset(maskAndModeM,    0, sizeof(maskAndModeM));
  memset(maskAndNotModeM, 0, sizeof(maskAndNotModeM));

  filterValueM[0] = tidP;
  filterMaskM[0] = maskP;

  // Invert the filter
  for (i = 0; i < DMX_MAX_FILTER_SIZE; ++i)
      filterValueM[i] ^= 0xFF;

  uint8_t mask, mode, doneq = 0;
  for (i = 0; i < DMX_MAX_FILTER_SIZE; ++i) {
      mode = filterModeM[i];
      mask = filterMaskM[i];
      maskAndModeM[i] = (uint8_t)(mask & mode);
      maskAndNotModeM[i] = (uint8_t)(mask & ~mode);
      doneq |= maskAndNotModeM[i];
      }
  doneqM = doneq ? 1 : 0;

  // Create filtering buffer
  ringbufferM = new cRingBufferLinear(KILOBYTE(128), 0, false, *cString::sprintf("IPTV SECTION %d/%d", devIdM, pidM));
  if (ringbufferM)
     ringbufferM->SetTimeouts(10, 10);
  else
     error("Failed to allocate buffer for section filter (device=%d pid=%d): ", devIdM, pidM);
}

cIptvSectionFilter::~cIptvSectionFilter()
{
  //debug("cIptvSectionFilter::~cIptvSectionfilter(%d, %d)\n", devIdM, pidM);
  DELETE_POINTER(ringbufferM);
  secBufM = NULL;
}

int cIptvSectionFilter::Read(void *Data, size_t Length)
{
  int count = 0;
  uchar *p = ringbufferM->Get(count);
  if (p && count > 0) {
     memcpy(Data, p, count);
     ringbufferM->Del(count);
     }
  return count;
}

inline uint16_t cIptvSectionFilter::GetLength(const uint8_t *dataP)
{
  return (uint16_t)(3 + ((dataP[1] & 0x0f) << 8) + dataP[2]);
}

void cIptvSectionFilter::New(void)
{
  tsFeedpM = secBufpM = secLenM = 0;
  secBufM = secBufBaseM;
}

int cIptvSectionFilter::Filter(void)
{
  if (secBufM) {
     int i;
     uint8_t neq = 0;

     for (i = 0; i < DMX_MAX_FILTER_SIZE; ++i) {
         uint8_t calcxor = (uint8_t)(filterValueM[i] ^ secBufM[i]);
         if (maskAndModeM[i] & calcxor)
            return 0;
         neq |= (maskAndNotModeM[i] & calcxor);
         }

     if (doneqM && !neq)
        return 0;

     if (ringbufferM) {
        int len = ringbufferM->Put(secBufM, secLenM);
        if (len != secLenM)
           ringbufferM->ReportOverflow(secLenM - len);
        // Update statistics
        AddSectionStatistic(len, 1);
        }
     }
  return 0;
}

inline int cIptvSectionFilter::Feed(void)
{
  if (Filter() < 0)
     return -1;
  secLenM = 0;
  return 0;
}

int cIptvSectionFilter::CopyDump(const uint8_t *bufP, uint8_t lenP)
{
  uint16_t limit, seclen, n;

  if (tsFeedpM >= DMX_MAX_SECFEED_SIZE)
     return 0;

  if (tsFeedpM + lenP > DMX_MAX_SECFEED_SIZE)
     lenP = (uint8_t)(DMX_MAX_SECFEED_SIZE - tsFeedpM);

  if (lenP <= 0)
     return 0;

  memcpy(secBufBaseM + tsFeedpM, bufP, lenP);
  tsFeedpM = uint16_t(tsFeedpM + lenP);

  limit = tsFeedpM;
  if (limit > DMX_MAX_SECFEED_SIZE)
     return -1; // internal error should never happen

  // Always set secbuf
  secBufM = secBufBaseM + secBufpM;

  for (n = 0; secBufpM + 2 < limit; ++n) {
      seclen = GetLength(secBufM);
      if ((seclen <= 0) || (seclen > DMX_MAX_SECTION_SIZE) || ((seclen + secBufpM) > limit))
         return 0;
      secLenM = seclen;
      if (pusiSeenM)
         Feed();
      secBufpM = uint16_t(secBufpM + seclen);
      secBufM += seclen;
      }
  return 0;
}

void cIptvSectionFilter::Process(const uint8_t* dataP)
{
  if (dataP[0] != TS_SYNC_BYTE)
     return;

  // Stop if not the PID this filter is looking for
  if (ts_pid(dataP) != pidM)
     return;

  uint8_t count = payload(dataP);

  // Check if no payload or out of range
  if (count == 0)
     return;

  // Payload start
  uint8_t p = (uint8_t)(TS_SIZE - count);

  uint8_t cc = (uint8_t)(dataP[3] & 0x0f);
  int ccok = ((feedCcM + 1) & 0x0f) == cc;
  feedCcM = cc;

  int dc_i = 0;
  if (dataP[3] & 0x20) {
     // Adaption field present, check for discontinuity_indicator
     if ((dataP[4] > 0) && (dataP[5] & 0x80))
        dc_i = 1;
     }

  if (!ccok || dc_i) {
     // Discontinuity detected. Reset pusiSeenM = 0 to
     // stop feeding of suspicious data until next PUSI=1 arrives
     pusiSeenM = 0;
     New();
     }

  if (dataP[1] & 0x40) {
     // PUSI=1 (is set), section boundary is here
     if (count > 1 && dataP[p] < count) {
        const uint8_t *before = &dataP[p + 1];
        uint8_t before_len = dataP[p];
        const uint8_t *after = &before[before_len];
        uint8_t after_len = (uint8_t)(count - 1 - before_len);
        CopyDump(before, before_len);

        // Before start of new section, set pusiSeenM = 1
        pusiSeenM = 1;
        New();
        CopyDump(after, after_len);
        }
     }
  else {
     // PUSI=0 (is not set), no section boundary
     CopyDump(&dataP[p], count);
     }
}
