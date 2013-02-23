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
#include "protocolcurl.h"
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
  unsigned int deviceIndexM;
  int dvrFdM;
  bool isPacketDeliveredM;
  bool isOpenDvrM;
  bool sidScanEnabledM;
  bool pidScanEnabledM;
  cRingBufferLinear *tsBufferM;
  mutable int tsBufferPrefillM;
  tChannelID channelIdM;
  cIptvProtocolUdp *pUdpProtocolM;
  cIptvProtocolCurl *pCurlProtocolM;
  cIptvProtocolHttp *pHttpProtocolM;
  cIptvProtocolFile *pFileProtocolM;
  cIptvProtocolExt *pExtProtocolM;
  cIptvStreamer *pIptvStreamerM;
  cPidScanner *pPidScannerM;
  cSidScanner *pSidScannerM;
  cMutex mutexM;
  cIptvSectionFilter *secFiltersM[eMaxSecFilterCount];

  // constructor & destructor
public:
  cIptvDevice(unsigned int deviceIndexP);
  virtual ~cIptvDevice();
  cString GetInformation(unsigned int pageP = IPTV_DEVICE_INFO_ALL);

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
  bool IsBuffering(void) const;
  bool DeleteFilter(unsigned int indexP);
  bool IsBlackListed(u_short pidP, u_char tidP, u_char maskP) const;

  // for channel info
public:
  virtual cString DeviceType(void) const;
  virtual cString DeviceName(void) const;
  virtual int SignalStrength(void) const;
  virtual int SignalQuality(void) const;

  // for channel selection
public:
  virtual bool ProvidesSource(int sourceP) const;
  virtual bool ProvidesTransponder(const cChannel *channelP) const;
  virtual bool ProvidesChannel(const cChannel *channelP, int priorityP = -1, bool *needsDetachReceiversP = NULL) const;
  virtual bool ProvidesEIT(void) const;
  virtual int NumProvidedSystems(void) const;
protected:
  virtual bool SetChannelDevice(const cChannel *channelP, bool liveViewP);

  // for recording
protected:
  virtual bool SetPid(cPidHandle *handleP, int typeP, bool onP);
  virtual bool OpenDvr(void);
  virtual void CloseDvr(void);
  virtual bool GetTSPacket(uchar *&dataP);

  // for section filtering
public:
  virtual int OpenFilter(u_short pidP, u_char tidP, u_char maskP);
  virtual int ReadFilter(int handleP, void *bufferP, size_t lengthP);
  virtual void CloseFilter(int handleP);

  // for transponder lock
public:
  virtual bool HasLock(int timeoutMsP) const;

  // for common interface
public:
  virtual bool HasInternalCam(void);
};

#endif // __IPTV_DEVICE_H
