/*
 * device.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: device.h,v 1.18 2007/09/26 19:49:35 rahrenbe Exp $
 */

#ifndef __IPTV_DEVICE_H
#define __IPTV_DEVICE_H

#include <vdr/device.h>
#include "protocoludp.h"
#include "protocolrtp.h"
#include "protocolhttp.h"
#include "protocolfile.h"
#include "streamer.h"
#include "sectionfilter.h"

class cIptvDevice : public cDevice {
  // static ones
public:
  static unsigned int deviceCount;
  static bool Initialize(unsigned int DeviceCount);
  static unsigned int Count(void);
  static cIptvDevice *Get(unsigned int DeviceIndex);

  // private parts
private:
  enum {
    eMaxSecFilterCount = 32
  };
  unsigned int deviceIndex;
  bool isPacketDelivered;
  bool isOpenDvr;
  cRingBufferLinear *tsBuffer;
  int tsBufferPrefill;
  cIptvProtocolUdp *pUdpProtocol;
  cIptvProtocolRtp *pRtpProtocol;
  cIptvProtocolHttp *pHttpProtocol;
  cIptvProtocolFile *pFileProtocol;
  cIptvStreamer *pIptvStreamer;
  cMutex mutex;
  cIptvSectionFilter* secfilters[eMaxSecFilterCount];

  // constructor & destructor
public:
  cIptvDevice(unsigned int DeviceIndex);
  virtual ~cIptvDevice();

  // for channel parsing & buffering
private:
  cString GetChannelSettings(const char *Param, int *IpPort, cIptvProtocolIf* *Protocol);
  bool ProvidesIptv(const char *Param) const;
  void ResetBuffering(void);
  bool IsBuffering(void);
  bool DeleteFilter(unsigned int Index);

  // for channel selection
public:
  virtual bool ProvidesSource(int Source) const;
  virtual bool ProvidesTransponder(const cChannel *Channel) const;
  virtual bool ProvidesChannel(const cChannel *Channel, int Priority = -1, bool *NeedsDetachReceivers = NULL) const;
protected:
  virtual bool SetChannelDevice(const cChannel *Channel, bool LiveView);

  // for recording
protected:
  virtual bool SetPid(cPidHandle *Handle, int Type, bool On);
  virtual bool OpenDvr(void);
  virtual void CloseDvr(void);
  virtual bool GetTSPacket(uchar *&Data);

  // for section filtering
public:
  virtual int OpenFilter(u_short Pid, u_char Tid, u_char Mask);
  virtual bool CloseFilter(int Handle);

  // for transponder lock
public:
  virtual bool HasLock(int);
};

#endif // __IPTV_DEVICE_H
