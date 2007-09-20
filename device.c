/*
 * device.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: device.c,v 1.28 2007/09/20 21:15:08 rahrenbe Exp $
 */

#include "common.h"
#include "config.h"
#include "device.h"

#define IPTV_FILTER_FILENAME "/tmp/vdr-iptv%d.filter%d"

#define IPTV_MAX_DEVICES 8

cIptvDevice * IptvDevices[IPTV_MAX_DEVICES];

unsigned int cIptvDevice::deviceCount = 0;

cIptvDevice::cIptvDevice(unsigned int Index)
: deviceIndex(Index),
  isPacketDelivered(false),
  isOpenDvr(false),
  mutex()
{
  debug("cIptvDevice::cIptvDevice(%d)\n", deviceIndex);
  tsBuffer = new cRingBufferLinear(MEGABYTE(IptvConfig.GetTsBufferSize()),
                                  (TS_SIZE * 2), false, "IPTV");
  tsBuffer->SetTimeouts(100, 100);
  // pad prefill to multiple of TS_SIZE
  tsBufferPrefill = MEGABYTE(IptvConfig.GetTsBufferSize()) *
                    IptvConfig.GetTsBufferPrefillRatio() / 100;
  tsBufferPrefill -= (tsBufferPrefill % TS_SIZE);
  //debug("Buffer=%d Prefill=%d\n", MEGABYTE(IptvConfig.GetTsBufferSize()), tsBufferPrefill);
  pUdpProtocol = new cIptvProtocolUdp();
  pHttpProtocol = new cIptvProtocolHttp();
  pFileProtocol = new cIptvProtocolFile();
  pIptvStreamer = new cIptvStreamer(tsBuffer, &mutex);
  // Initialize filters  
  memset(&filter, '\0', sizeof(filter));
  init_trans(&filter);
  for (int i = 0; i < eMaxFilterCount; ++i) {
      struct stat sb;
      snprintf(filters[i].pipeName, sizeof(filters[i].pipeName), IPTV_FILTER_FILENAME, deviceIndex, i);
      stat(filters[i].pipeName, &sb);
      if (S_ISFIFO(sb.st_mode))
         unlink(filters[i].pipeName);
      memset(filters[i].pipeName, '\0', sizeof(filters[i].pipeName));
      filters[i].fifoDesc = -1;
      filters[i].active = false;
      }
  StartSectionHandler();
}

cIptvDevice::~cIptvDevice()
{
  debug("cIptvDevice::~cIptvDevice(%d)\n", deviceIndex);
  delete pIptvStreamer;
  delete pUdpProtocol;
  delete tsBuffer;
  // Iterate over all filters and clear their settings 
  for (int i = 0; i < eMaxFilterCount; ++i) {
      if (filters[i].active) {
         close(filters[i].fifoDesc);
         unlink(filters[i].pipeName);
         memset(filters[i].pipeName, '\0', sizeof(filters[i].pipeName));
         filters[i].fifoDesc = -1;
         filters[i].active = false;
         clear_trans_filt(&filter, i);
         }
      }
}

bool cIptvDevice::Initialize(unsigned int DeviceCount)
{
  debug("cIptvDevice::Initialize()\n");
  if (DeviceCount > IPTV_MAX_DEVICES)
     DeviceCount = IPTV_MAX_DEVICES;
  for (unsigned int i = 0; i < DeviceCount; ++i)
      IptvDevices[i] = new cIptvDevice(i);
  return true;
}

unsigned int cIptvDevice::Count(void)
{
  unsigned int count = 0;
  debug("cIptvDevice::Count()\n");
  for (unsigned int i = 0; i < IPTV_MAX_DEVICES; ++i) {
      if (IptvDevices[i])
         count++;
      }
  return count;
}

cIptvDevice *cIptvDevice::Get(unsigned int DeviceIndex)
{
  debug("cIptvDevice::Get()\n");
  if ((DeviceIndex > 0) && (DeviceIndex <= IPTV_MAX_DEVICES))
     return IptvDevices[DeviceIndex - 1];
  return NULL;
}

cString cIptvDevice::GetChannelSettings(const char *Param, int *IpPort, cIptvProtocolIf* *Protocol)
{
  debug("cIptvDevice::GetChannelSettings(%d)\n", deviceIndex);
  char *loc = NULL;
  if (sscanf(Param, "IPTV|UDP|%a[^|]|%u", &loc, IpPort) == 2) {
     cString addr(loc, true);
     *Protocol = pUdpProtocol;
     return addr;
     }
  else if (sscanf(Param, "IPTV|HTTP|%a[^|]|%u", &loc, IpPort) == 2) {
     cString addr(loc, true);
     *Protocol = pHttpProtocol;
     return addr;
     }
  else if (sscanf(Param, "IPTV|FILE|%a[^|]|%u", &loc, IpPort) == 2) {
     cString addr(loc, true);
     *Protocol = pFileProtocol;
     return addr;
     }
  return NULL;
}

bool cIptvDevice::ProvidesIptv(const char *Param) const
{
  debug("cIptvDevice::ProvidesIptv(%d)\n", deviceIndex);
  return (strncmp(Param, "IPTV", 4) == 0);
}

bool cIptvDevice::ProvidesSource(int Source) const
{
  debug("cIptvDevice::ProvidesSource(%d)\n", deviceIndex);
  return (cSource::IsPlug(Source));
}

bool cIptvDevice::ProvidesTransponder(const cChannel *Channel) const
{
  debug("cIptvDevice::ProvidesTransponder(%d)\n", deviceIndex);
  return (ProvidesSource(Channel->Source()) && ProvidesIptv(Channel->PluginParam()));
}

bool cIptvDevice::ProvidesChannel(const cChannel *Channel, int Priority, bool *NeedsDetachReceivers) const
{
  bool result = false;
  bool needsDetachReceivers = false;

  debug("cIptvDevice::ProvidesChannel(%d)\n", deviceIndex);
  if (ProvidesTransponder(Channel))
     result = true;
  if (NeedsDetachReceivers)
     *NeedsDetachReceivers = needsDetachReceivers;
  return result;
}

bool cIptvDevice::SetChannelDevice(const cChannel *Channel, bool LiveView)
{
  int port;
  cString addr;
  cIptvProtocolIf *protocol;

  debug("cIptvDevice::SetChannelDevice(%d)\n", deviceIndex);
  addr = GetChannelSettings(Channel->PluginParam(), &port, &protocol);
  if (isempty(addr)) {
     error("ERROR: Unrecognized IPTV channel settings: %s", Channel->PluginParam());
     return false;
     }
  // pad prefill to multiple of TS_SIZE
  tsBufferPrefill = MEGABYTE(IptvConfig.GetTsBufferSize()) *
                    IptvConfig.GetTsBufferPrefillRatio() / 100;
  tsBufferPrefill -= (tsBufferPrefill % TS_SIZE);
  pIptvStreamer->Set(addr, port, protocol);
  return true;
}

bool cIptvDevice::SetPid(cPidHandle *Handle, int Type, bool On)
{
  debug("cIptvDevice::SetPid(%d) Pid=%d Type=%d On=%d\n", deviceIndex, Handle->pid, Type, On);
  return true;
}

int cIptvDevice::OpenFilter(u_short Pid, u_char Tid, u_char Mask)
{
  // Search the next free filter slot
  for (unsigned int i = 0; i < eMaxFilterCount; ++i) {
      if (!filters[i].active) {
         debug("cIptvDevice::OpenFilter(%d): Pid=%d Tid=%02X Mask=%02X Count=%d\n", deviceIndex, Pid, Tid, Mask, i);
         uint8_t mask[eMaxFilterMaskLen] = { 0 };
         uint8_t filt[eMaxFilterMaskLen] = { 0 };
         mask[0] = Mask;
         filt[0] = Tid;
         int err = set_trans_filt(&filter, i, Pid, &mask[0], &filt[0], 0);
         if (err < 0)
            error("Cannot set filter %d\n", i);
         memset(filters[i].pipeName, '\0', sizeof(filters[i].pipeName));
         snprintf(filters[i].pipeName, sizeof(filters[i].pipeName), IPTV_FILTER_FILENAME, deviceIndex, i);
         struct stat sb;
         stat(filters[i].pipeName, &sb);
         if (S_ISFIFO(sb.st_mode))
            unlink(filters[i].pipeName);
         err = mknod(filters[i].pipeName, 0644 | S_IFIFO, 0);
         if (err < 0) {
            char tmp[64];
            error("ERROR: mknod(): %s", strerror_r(errno, tmp, sizeof(tmp)));
            break;
            }
         // Create descriptors
         int fifoDescriptor   = open(filters[i].pipeName, O_RDWR | O_NONBLOCK);
         int returnDescriptor = open(filters[i].pipeName, O_RDONLY | O_NONBLOCK);
         // Store the write pipe and set active flag
         filters[i].fifoDesc = fifoDescriptor;
         filters[i].active = true;
         return returnDescriptor;
         }
      }
  // No free filter slot found
  return -1;
}

bool cIptvDevice::OpenDvr(void)
{
  debug("cIptvDevice::OpenDvr(%d)\n", deviceIndex);
  mutex.Lock();
  isPacketDelivered = false;
  tsBuffer->Clear();
  mutex.Unlock();
  pIptvStreamer->Open();
  isOpenDvr = true;
  return true;
}

void cIptvDevice::CloseDvr(void)
{
  debug("cIptvDevice::CloseDvr(%d)\n", deviceIndex);
  pIptvStreamer->Close();
  // Iterate over all filters and clear their settings
  for (int i = 0; i < eMaxFilterCount; ++i) {
      if (filters[i].active) {
         close(filters[i].fifoDesc);
         unlink(filters[i].pipeName);
         memset(filters[i].pipeName, '\0', sizeof(filters[i].pipeName));
         filters[i].fifoDesc = -1;
         filters[i].active = false;
         clear_trans_filt(&filter, i);
         }
      }
  isOpenDvr = false;
}

bool cIptvDevice::GetTSPacket(uchar *&Data)
{
  int Count = 0;
  //debug("cIptvDevice::GetTSPacket(%d)\n", deviceIndex);
  if (tsBufferPrefill && tsBuffer->Available() < tsBufferPrefill)
     return false;
  else
     tsBufferPrefill = 0;
  if (isPacketDelivered) {
     tsBuffer->Del(TS_SIZE);
     isPacketDelivered = false;
     }
  uchar *p = tsBuffer->Get(Count);
  if (p && Count >= TS_SIZE) {
     if (*p != TS_SYNC_BYTE) {
        for (int i = 1; i < Count; i++) {
            if (p[i] == TS_SYNC_BYTE) {
               Count = i;
               break;
               }
            }
        tsBuffer->Del(Count);
        error("ERROR: skipped %d bytes to sync on TS packet\n", Count);
        return false;
        }
     isPacketDelivered = true;
     Data = p;
     memcpy(filter.packet, p, sizeof(filter.packet));
     trans_filt(p, TS_SIZE, &filter);
     for (unsigned int i = 0; i < eMaxFilterCount; ++i) {
         if (filters[i].active) {
	    section *filtered = get_filt_sec(&filter, i);
            if (filtered->found) {
               // Select on the fifo emptyness. Using null timeout to return
	       // immediately
               struct timeval tv;
               tv.tv_sec = 0;
               tv.tv_usec = 0;
               fd_set rfds;
               FD_ZERO(&rfds);
               FD_SET(filters[i].fifoDesc, &rfds);
               int retval = select(filters[i].fifoDesc + 1, &rfds, NULL, NULL, &tv);
               // Check if error
               if (retval < 0) {
                  char tmp[64];
                  error("ERROR: select(): %s", strerror_r(errno, tmp, sizeof(tmp)));
                  // VDR has probably closed the filter file descriptor, so clear
                  // the filter
                  close(filters[i].fifoDesc);
                  unlink(filters[i].pipeName);
                  memset(filters[i].pipeName, '\0', sizeof(filters[i].pipeName));
                  filters[i].fifoDesc = -1;
                  filters[i].active = false;
                  clear_trans_filt(&filter, i);	     
                  }
               // There is no data in the fifo, more can be written
               else if (!retval) {
                  int err = write(filters[i].fifoDesc, filtered->payload, filtered->length + 3);
                  if (err < 0) {
                     char tmp[64];
                     error("ERROR: write(): %s", strerror_r(errno, tmp, sizeof(tmp)));
                     }
                 }
               }
            }
         }
     return true;
     }
  Data = NULL;
  return true;
}

