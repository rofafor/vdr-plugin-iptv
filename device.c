/*
 * device.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "config.h"
#include "source.h"
#include "device.h"

#define IPTV_MAX_DEVICES MAXDEVICES

static cIptvDevice * IptvDevicesS[IPTV_MAX_DEVICES] = { NULL };

cIptvDevice::cIptvDevice(unsigned int indexP)
: deviceIndexM(indexP),
  dvrFdM(-1),
  isPacketDeliveredM(false),
  isOpenDvrM(false),
  sidScanEnabledM(false),
  pidScanEnabledM(false),
  channelM()
{
  unsigned int bufsize = (unsigned int)MEGABYTE(IptvConfig.GetTsBufferSize());
  bufsize -= (bufsize % TS_SIZE);
  isyslog("creating IPTV device %d (CardIndex=%d)", deviceIndexM, CardIndex());
  tsBufferM = new cRingBufferLinear(bufsize + 1, TS_SIZE, false,
                                   *cString::sprintf("IPTV TS %d", deviceIndexM));
  if (tsBufferM) {
     tsBufferM->SetTimeouts(100, 100);
     tsBufferM->SetIoThrottle();
     pIptvStreamerM = new cIptvStreamer(*this, tsBufferM->Free());
     }
  ResetBuffering();
  pUdpProtocolM = new cIptvProtocolUdp();
  pCurlProtocolM = new cIptvProtocolCurl();
  pHttpProtocolM = new cIptvProtocolHttp();
  pFileProtocolM = new cIptvProtocolFile();
  pExtProtocolM = new cIptvProtocolExt();
  pPidScannerM = new cPidScanner();
  // Start section handler for iptv device
  pIptvSectionM = new cIptvSectionFilterHandler(deviceIndexM, bufsize + 1);
  StartSectionHandler();
  // Sid scanner must be created after the section handler
  AttachFilter(pSidScannerM = new cSidScanner());
  // Check if dvr fifo exists
  struct stat sb;
  cString filename = cString::sprintf(IPTV_DVR_FILENAME, deviceIndexM);
  stat(filename, &sb);
  if (S_ISFIFO(sb.st_mode)) {
     dvrFdM = open(filename, O_RDWR | O_NONBLOCK);
     if (dvrFdM >= 0)
        dsyslog("IPTV device %d redirecting input stream to '%s'", deviceIndexM, *filename);
     }
}

cIptvDevice::~cIptvDevice()
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  // Stop section handler of iptv device
  StopSectionHandler();
  DELETE_POINTER(pIptvSectionM);
  DELETE_POINTER(pSidScannerM);
  DELETE_POINTER(pPidScannerM);
  DELETE_POINTER(pIptvStreamerM);
  DELETE_POINTER(pExtProtocolM);
  DELETE_POINTER(pFileProtocolM);
  DELETE_POINTER(pHttpProtocolM);
  DELETE_POINTER(pCurlProtocolM);
  DELETE_POINTER(pUdpProtocolM);
  DELETE_POINTER(tsBufferM);
  // Close dvr fifo
  if (dvrFdM >= 0) {
     int fd = dvrFdM;
     dvrFdM = -1;
     close(fd);
     }
}

bool cIptvDevice::Initialize(unsigned int deviceCountP)
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceCountP);
  new cIptvSourceParam(IPTV_SOURCE_CHARACTER, "IPTV");
  if (deviceCountP > IPTV_MAX_DEVICES)
     deviceCountP = IPTV_MAX_DEVICES;
  for (unsigned int i = 0; i < deviceCountP; ++i)
      IptvDevicesS[i] = new cIptvDevice(i);
  for (unsigned int i = deviceCountP; i < IPTV_MAX_DEVICES; ++i)
      IptvDevicesS[i] = NULL;
  return true;
}

void cIptvDevice::Shutdown(void)
{
  debug("cIptvDevice::%s()", __FUNCTION__);
  for (int i = 0; i < IPTV_MAX_DEVICES; ++i) {
      if (IptvDevicesS[i])
         IptvDevicesS[i]->CloseDvr();
      }
}

unsigned int cIptvDevice::Count(void)
{
  unsigned int count = 0;
  debug("cIptvDevice::%s()", __FUNCTION__);
  for (unsigned int i = 0; i < IPTV_MAX_DEVICES; ++i) {
      if (IptvDevicesS[i] != NULL)
         count++;
      }
  return count;
}

cIptvDevice *cIptvDevice::GetIptvDevice(int cardIndexP)
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, cardIndexP);
  for (unsigned int i = 0; i < IPTV_MAX_DEVICES; ++i) {
      if (IptvDevicesS[i] && (IptvDevicesS[i]->CardIndex() == cardIndexP)) {
         //debug("cIptvDevice::%s(%d): found!", __FUNCTION__, cardIndexP);
         return IptvDevicesS[i];
         }
      }
  return NULL;
}

cString cIptvDevice::GetGeneralInformation(void)
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return cString::sprintf("IPTV device: %d\nCardIndex: %d\nStream: %s\nStream bitrate: %s\n%sChannel: %s",
                          deviceIndexM, CardIndex(),
                          pIptvStreamerM ? *pIptvStreamerM->GetInformation() : "",
                          pIptvStreamerM ? *pIptvStreamerM->GetStreamerStatistic() : "",
                          *GetBufferStatistic(),
                          *Channels.GetByNumber(cDevice::CurrentChannel())->ToText());
}

cString cIptvDevice::GetPidsInformation(void)
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return GetPidStatistic();
}

cString cIptvDevice::GetFiltersInformation(void)
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return cString::sprintf("Active section filters:\n%s", pIptvSectionM ? *pIptvSectionM->GetInformation() : "");
}

cString cIptvDevice::GetInformation(unsigned int pageP)
{
  // generate information string
  cString s;
  switch (pageP) {
    case IPTV_DEVICE_INFO_GENERAL:
         s = GetGeneralInformation();
         break;
    case IPTV_DEVICE_INFO_PIDS:
         s = GetPidsInformation();
         break;
    case IPTV_DEVICE_INFO_FILTERS:
         s = GetFiltersInformation();
         break;
    case IPTV_DEVICE_INFO_PROTOCOL:
         s = pIptvStreamerM ? *pIptvStreamerM->GetInformation() : "";
         break;
    case IPTV_DEVICE_INFO_BITRATE:
         s = pIptvStreamerM ? *pIptvStreamerM->GetStreamerStatistic() : "";
         break;
    default:
         s = cString::sprintf("%s%s%s",
                              *GetGeneralInformation(),
                              *GetPidsInformation(),
                              *GetFiltersInformation());
         break;
    }
  return s;
}

cString cIptvDevice::DeviceType(void) const
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return "IPTV";
}

cString cIptvDevice::DeviceName(void) const
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return cString::sprintf("IPTV %d", deviceIndexM);
}

int cIptvDevice::SignalStrength(void) const
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return (100);
}

int cIptvDevice::SignalQuality(void) const
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return (100);
}

bool cIptvDevice::ProvidesSource(int sourceP) const
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return (cSource::IsType(sourceP, IPTV_SOURCE_CHARACTER));
}

bool cIptvDevice::ProvidesTransponder(const cChannel *channelP) const
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return (ProvidesSource(channelP->Source()));
}

bool cIptvDevice::ProvidesChannel(const cChannel *channelP, int priorityP, bool *needsDetachReceiversP) const
{
  bool result = false;
  bool hasPriority = (priorityP == IDLEPRIORITY) || (priorityP > this->Priority());
  bool needsDetachReceivers = false;

  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);

  if (channelP && ProvidesTransponder(channelP)) {
     result = hasPriority;
     if (Receiving()) {
        if (channelP->GetChannelID() == channelM.GetChannelID())
           result = true;
        else
           needsDetachReceivers = Receiving();
        }
     }
  if (needsDetachReceiversP)
     *needsDetachReceiversP = needsDetachReceivers;
  return result;
}

bool cIptvDevice::ProvidesEIT(void) const
{
  return false;
}

int cIptvDevice::NumProvidedSystems(void) const
{
  return 1;
}

const cChannel *cIptvDevice::GetCurrentlyTunedTransponder(void) const
{
  return &channelM;
}

bool cIptvDevice::IsTunedToTransponder(const cChannel *channelP) const
{
  return channelP ? (channelP->GetChannelID() == channelM.GetChannelID()) : false;
}

bool cIptvDevice::MaySwitchTransponder(const cChannel *channelP) const
{
  return cDevice::MaySwitchTransponder(channelP);
}

bool cIptvDevice::SetChannelDevice(const cChannel *channelP, bool liveViewP)
{
  cIptvProtocolIf *protocol;
  cIptvTransponderParameters itp(channelP->Parameters());

  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);

  if (isempty(itp.Address())) {
     error("Unrecognized IPTV address: %s", channelP->Parameters());
     return false;
     }
  switch (itp.Protocol()) {
    case cIptvTransponderParameters::eProtocolUDP:
         protocol = pUdpProtocolM;
         break;
    case cIptvTransponderParameters::eProtocolCURL:
         protocol = pCurlProtocolM;
         break;
    case cIptvTransponderParameters::eProtocolHTTP:
         protocol = pHttpProtocolM;
         break;
    case cIptvTransponderParameters::eProtocolFILE:
         protocol = pFileProtocolM;
         break;
    case cIptvTransponderParameters::eProtocolEXT:
         protocol = pExtProtocolM;
         break;
    default:
         error("Unrecognized IPTV protocol: %s", channelP->Parameters());
         return false;
         break;
  }
  sidScanEnabledM = itp.SidScan() ? true : false;
  pidScanEnabledM = itp.PidScan() ? true : false;
  if (pIptvStreamerM->Set(itp.Address(), itp.Parameter(), deviceIndexM, protocol)) {
     channelM = *channelP;
     if (sidScanEnabledM && pSidScannerM && IptvConfig.GetSectionFiltering())
        pSidScannerM->SetChannel(channelM.GetChannelID());
     if (pidScanEnabledM && pPidScannerM)
        pPidScannerM->SetChannel(channelM.GetChannelID());
     }
  return true;
}

bool cIptvDevice::SetPid(cPidHandle *handleP, int typeP, bool onP)
{
  debug("cIptvDevice::%s(%d): pid=%d type=%d on=%d", __FUNCTION__, deviceIndexM, handleP->pid, typeP, onP);
  return true;
}

int cIptvDevice::OpenFilter(u_short pidP, u_char tidP, u_char maskP)
{
  //debug("cIptvDevice::%s(%d): pid=%d tid=%d mask=%d", __FUNCTION__, deviceIndexM, pidP, tidP,  maskP);
  if (pIptvSectionM && IptvConfig.GetSectionFiltering())
     return pIptvSectionM->Open(pidP, tidP, maskP);
  return -1;
}

void cIptvDevice::CloseFilter(int handleP)
{
  //debug("cIptvDevice::%s(%d): handle=%d", __FUNCTION__, deviceIndexM, handleP);
  if (pIptvSectionM)
     pIptvSectionM->Close(handleP);
}

bool cIptvDevice::OpenDvr(void)
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  isPacketDeliveredM = false;
  tsBufferM->Clear();
  ResetBuffering();
  if (pIptvStreamerM)
     pIptvStreamerM->Open();
  if (sidScanEnabledM && pSidScannerM && IptvConfig.GetSectionFiltering())
     pSidScannerM->Open();
  if (pidScanEnabledM && pPidScannerM)
     pPidScannerM->Open();
  isOpenDvrM = true;
  return true;
}

void cIptvDevice::CloseDvr(void)
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  if (pidScanEnabledM && pPidScannerM)
     pPidScannerM->Close();
  if (sidScanEnabledM && pSidScannerM)
     pSidScannerM->Close();
  if (pIptvStreamerM)
     pIptvStreamerM->Close();
  isOpenDvrM = false;
}

bool cIptvDevice::HasLock(int timeoutMsP) const
{
  //debug("cIptvDevice::%s(%d): timeoutMs=%d", __FUNCTION__, deviceIndexM, timeoutMsP);
  return (!IsBuffering());
}

bool cIptvDevice::HasInternalCam(void)
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  return true;
}

void cIptvDevice::ResetBuffering(void)
{
  debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  // Pad prefill to multiple of TS_SIZE
  tsBufferPrefillM = (unsigned int)MEGABYTE(IptvConfig.GetTsBufferSize()) *
                    IptvConfig.GetTsBufferPrefillRatio() / 100;
  tsBufferPrefillM -= (tsBufferPrefillM % TS_SIZE);
}

bool cIptvDevice::IsBuffering(void) const
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  if (tsBufferPrefillM && tsBufferM && tsBufferM->Available() < tsBufferPrefillM)
     return true;
  else
     tsBufferPrefillM = 0;
  return false;
}

void cIptvDevice::WriteData(uchar *bufferP, int lengthP)
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  int len;
  // Send data to dvr fifo
  if (dvrFdM >= 0)
     len = write(dvrFdM, bufferP, lengthP);
  // Fill up TS buffer
  if (tsBufferM) {
     len = tsBufferM->Put(bufferP, lengthP);
     if (len != lengthP)
        tsBufferM->ReportOverflow(lengthP - len);
     }
  // Filter the sections
  if (pIptvSectionM && IptvConfig.GetSectionFiltering())
     pIptvSectionM->Write(bufferP, lengthP);
}

unsigned int cIptvDevice::CheckData(void)
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  if (tsBufferM)
     return (unsigned int)tsBufferM->Free();
  return 0;
}

bool cIptvDevice::GetTSPacket(uchar *&Data)
{
  //debug("cIptvDevice::%s(%d)", __FUNCTION__, deviceIndexM);
  if (isOpenDvrM && tsBufferM && !IsBuffering()) {
     if (isPacketDeliveredM) {
        tsBufferM->Del(TS_SIZE);
        isPacketDeliveredM = false;
        // Update buffer statistics
        AddBufferStatistic(TS_SIZE, tsBufferM->Available());
        }
     int Count = 0;
     uchar *p = tsBufferM->Get(Count);
     if (p && Count >= TS_SIZE) {
        if (*p != TS_SYNC_BYTE) {
           for (int i = 1; i < Count; i++) {
               if (p[i] == TS_SYNC_BYTE) {
                  Count = i;
                  break;
                  }
               }
           tsBufferM->Del(Count);
           error("Skipped %d bytes to sync on TS packet", Count);
           return false;
           }
        isPacketDeliveredM = true;
        Data = p;
        // Update pid statistics
        AddPidStatistic(ts_pid(p), payload(p));
        return true;
        }
     }
  // Reduce cpu load by preventing busylooping
  cCondWait::SleepMs(10);
  Data = NULL;
  return true;
}
