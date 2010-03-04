/*
 * device.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_DEVICE_H
#define __IPTV_DEVICE_H

#include <vdr/device.h>
#include "common.h"
#include "protocoludp.h"
#include "protocolhttp.h"
#include "protocolfile.h"
#include "protocolext.h"
#include "streamer.h"
#include "sectionfilter.h"
#include "pidscanner.h"
#include "sidscanner.h"
#include "statistics.h"

class cIptvDevice : public cDevice, public cIptvPidStatistics, public cIptvBufferStatistics {
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
  int dvrFd;
  bool isPacketDelivered;
  bool isOpenDvr;
  bool sidScanEnabled;
  bool pidScanEnabled;
  cRingBufferLinear *tsBuffer;
  int tsBufferPrefill;
  cIptvProtocolUdp *pUdpProtocol;
  cIptvProtocolHttp *pHttpProtocol;
  cIptvProtocolFile *pFileProtocol;
  cIptvProtocolExt *pExtProtocol;
  cIptvStreamer *pIptvStreamer;
  cPidScanner *pPidScanner;
  cSidScanner *pSidScanner;
  cMutex mutex;
  cIptvSectionFilter* secfilters[eMaxSecFilterCount];

  // constructor & destructor
public:
  cIptvDevice(unsigned int DeviceIndex);
  virtual ~cIptvDevice();
  cString GetInformation(unsigned int Page = IPTV_DEVICE_INFO_ALL);

  // copy and assignment constructors
private:
  cIptvDevice(const cIptvDevice&);
  cIptvDevice& operator=(const cIptvDevice&);

  // for statistics and general information
  cString GetGeneralInformation(void);
  cString GetPidsInformation(void);
  cString GetFiltersInformation(void);

  // for channel parsing & buffering
private:
  void ResetBuffering(void);
  bool IsBuffering(void);
  bool DeleteFilter(unsigned int Index);
  bool IsBlackListed(u_short Pid, u_char Tid, u_char Mask) const;

  // for channel selection
public:
  virtual bool ProvidesSource(int Source) const;
  virtual bool ProvidesTransponder(const cChannel *Channel) const;
  virtual bool ProvidesChannel(const cChannel *Channel, int Priority = -1, bool *NeedsDetachReceivers = NULL) const;
  virtual int NumProvidedSystems(void) const;
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
  virtual void CloseFilter(int Handle);

  // for transponder lock
public:
  virtual bool HasLock(int);
};

#endif // __IPTV_DEVICE_H
