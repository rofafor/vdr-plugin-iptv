/*
 * device.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: device.h,v 1.30 2007/10/09 17:58:17 ajhseppa Exp $
 */

#ifndef __IPTV_DEVICE_H
#define __IPTV_DEVICE_H

#include <vdr/device.h>
#include "common.h"
#include "protocoludp.h"
#include "protocolhttp.h"
#include "protocolfile.h"
#include "streamer.h"
#include "sectionfilter.h"
#include "sidscanner.h"
#include "statistics.h"

class cIptvDevice : public cDevice, public cIptvDeviceStatistics, public cIptvBufferStatistics {
  // static ones
public:
  static unsigned int deviceCount;
  static bool Initialize(unsigned int DeviceCount);
  static unsigned int Count(void);
  static cIptvDevice *GetIptvDevice(int CardIndex);

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
  cIptvProtocolHttp *pHttpProtocol;
  cIptvProtocolFile *pFileProtocol;
  cIptvStreamer *pIptvStreamer;
  cSidScanner *pSidScanner;
  cMutex mutex;
  cIptvSectionFilter* secfilters[eMaxSecFilterCount];

  // constructor & destructor
public:
  cIptvDevice(unsigned int DeviceIndex);
  virtual ~cIptvDevice();
  cString GetInformation(unsigned int Page = IPTV_DEVICE_INFO_ALL);

  // for statistics and general information
private:
  cString GetGeneralInformation(void);
  cString GetPidsInformation(void);
  cString GetFiltersInformation(void);

  // for channel parsing & buffering
private:
  cString GetChannelSettings(const char *Param, int *IpPort, cIptvProtocolIf* *Protocol);
  bool ProvidesIptv(const char *Param) const;
  void ResetBuffering(void);
  bool IsBuffering(void);
  bool DeleteFilter(unsigned int Index);
  bool IsBlackListed(u_short Pid, u_char Tid, u_char Mask);

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
