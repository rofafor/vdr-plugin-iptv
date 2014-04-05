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
#include "deviceif.h"
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

class cIptvDevice : public cDevice, public cIptvPidStatistics, public cIptvBufferStatistics, public cIptvDeviceIf {
  // static ones
public:
  static unsigned int deviceCount;
  static bool Initialize(unsigned int DeviceCount);
  static void Shutdown(void);
  static unsigned int Count(void);
  static cIptvDevice *GetIptvDevice(int CardIndex);

  // private parts
private:
  unsigned int deviceIndexM;
  int dvrFdM;
  bool isPacketDeliveredM;
  bool isOpenDvrM;
  bool sidScanEnabledM;
  bool pidScanEnabledM;
  cRingBufferLinear *tsBufferM;
  cChannel channelM;
  cIptvProtocolUdp *pUdpProtocolM;
  cIptvProtocolCurl *pCurlProtocolM;
  cIptvProtocolHttp *pHttpProtocolM;
  cIptvProtocolFile *pFileProtocolM;
  cIptvProtocolExt *pExtProtocolM;
  cIptvStreamer *pIptvStreamerM;
  cIptvSectionFilterHandler *pIptvSectionM;
  cPidScanner *pPidScannerM;
  cSidScanner *pSidScannerM;
  cMutex mutexM;

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
  virtual const cChannel *GetCurrentlyTunedTransponder(void) const;
  virtual bool IsTunedToTransponder(const cChannel *channelP) const;
  virtual bool MaySwitchTransponder(const cChannel *channelP) const;

protected:
  virtual bool SetChannelDevice(const cChannel *channelP, bool liveViewP);

  // for recording
private:
  uchar *GetData(int *availableP = NULL);
  void SkipData(int countP);

protected:
  virtual bool SetPid(cPidHandle *handleP, int typeP, bool onP);
  virtual bool OpenDvr(void);
  virtual void CloseDvr(void);
  virtual bool GetTSPacket(uchar *&dataP);

  // for section filtering
public:
  virtual int OpenFilter(u_short pidP, u_char tidP, u_char maskP);
  virtual void CloseFilter(int handleP);

  // for transponder lock
public:
  virtual bool HasLock(int timeoutMsP) const;

  // for common interface
public:
  virtual bool HasInternalCam(void);

  // for internal device interface
public:
  virtual void WriteData(u_char *bufferP, int lengthP);
  virtual unsigned int CheckData(void);
};

#endif // __IPTV_DEVICE_H
