/*
 * device.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: device.c,v 1.2 2007/09/12 18:55:31 rahrenbe Exp $
 */

#include "common.h"
#include "device.h"

#define IPTV_MAX_DEVICES 8

cIptvDevice * IptvDevices[IPTV_MAX_DEVICES];

unsigned int cIptvDevice::deviceCount = 0;

cIptvDevice::cIptvDevice(unsigned int Index)
: deviceIndex(Index),
  isPacketDelivered(false),
  isOpenDvr(false),
  pIptvStreamer(NULL)
{
  debug("cIptvDevice::cIptvDevice(%d)\n", deviceIndex);
  tsBuffer = new cRingBufferLinear(MEGABYTE(8), TS_SIZE, false, "IPTV");
  tsBuffer->SetTimeouts(100, 100);
  pIptvStreamer = new cIptvStreamer(tsBuffer, &mutex);
}

cIptvDevice::~cIptvDevice()
{
  debug("cIptvDevice::~cIptvDevice(%d)\n", deviceIndex);
  if (pIptvStreamer)
     delete pIptvStreamer;
  delete tsBuffer;
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

cString cIptvDevice::GetChannelSettings(const char *Param, int *IpPort, int *Protocol)
{
  unsigned int a, b, c, d;

  debug("cIptvDevice::GetChannelSettings(%d)\n", deviceIndex);
  if (sscanf(Param, "IPTV-UDP-%u.%u.%u.%u-%u", &a, &b, &c, &d, IpPort) == 5) {
     debug("UDP channel detected\n");
     Protocol = (int*)(cIptvStreamer::PROTOCOL_UDP);
     return cString::sprintf("%u.%u.%u.%u", a, b, c, d);
     }
  else if (sscanf(Param, "IPTV-RTSP-%u.%u.%u.%u-%u", &a, &b, &c, &d, IpPort) == 5) {
     debug("RTSP channel detected\n");
     Protocol = (int*)(cIptvStreamer::PROTOCOL_RTSP);
     return cString::sprintf("%u.%u.%u.%u", a, b, c, d);
     }
  else if (sscanf(Param, "IPTV-HTTP-%u.%u.%u.%u-%u", &a, &b, &c, &d, IpPort) == 5) {
     debug("HTTP channel detected\n");
     Protocol = (int*)cIptvStreamer::PROTOCOL_HTTP;
     return cString::sprintf("%u.%u.%u.%u", a, b, c, d);
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
  return ((Source & cSource::st_Mask) == cSource::stPlug);
}

bool cIptvDevice::ProvidesTransponder(const cChannel *Channel) const
{
  debug("cIptvDevice::ProvidesTransponder(%d)\n", deviceIndex);
  return (ProvidesSource(Channel->Source()) && ProvidesIptv(Channel->Param()));
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
  int port, protocol;
  cString addr;

  debug("cIptvDevice::SetChannelDevice(%d)\n", deviceIndex);
  addr = GetChannelSettings(Channel->Param(), &port, &protocol);
  if (addr)
     pIptvStreamer->SetStream(addr, port, protocol);
  return true;
}

bool cIptvDevice::SetPid(cPidHandle *Handle, int Type, bool On)
{
  debug("cIptvDevice::SetPid(%d)\n", deviceIndex);
  return true;
}

bool cIptvDevice::OpenDvr(void)
{
  debug("cIptvDevice::OpenDvr(%d)\n", deviceIndex);
  mutex.Lock();
  isPacketDelivered = false;
  tsBuffer->Clear();
  mutex.Unlock();
  pIptvStreamer->Activate();
  isOpenDvr = true;
  return true;
}

void cIptvDevice::CloseDvr(void)
{
  debug("cIptvDevice::CloseDvr(%d)\n", deviceIndex);
  pIptvStreamer->Deactivate();
  isOpenDvr = false;
}

bool cIptvDevice::GetTSPacket(uchar *&Data)
{
  int Count = 0;
  //debug("cIptvDevice::GetTSPacket(%d)\n", deviceIndex);
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
     return true;
     }
  Data = NULL;
  return true;
}

