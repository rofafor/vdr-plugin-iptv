/*
 * protocolcurl.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "config.h"
#include "log.h"
#include "protocolcurl.h"

#ifdef CURLOPT_RTSPHEADER
#define USE_RTSP
#endif

#define iptv_curl_easy_setopt(X, Y, Z) \
  if ((res = curl_easy_setopt((X), (Y), (Z))) != CURLE_OK) { \
     error("curl_easy_setopt(%s, %s) failed: %s (%d)", #Y, #Z, curl_easy_strerror(res), res); \
     }

#define iptv_curl_easy_perform(X) \
  if ((res = curl_easy_perform((X))) != CURLE_OK) { \
     error("curl_easy_perform() failed: %s (%d)", curl_easy_strerror(res), res); \
     }

cIptvProtocolCurl::cIptvProtocolCurl()
: streamUrlM(""),
  streamParamM(0),
  streamPortM(0),
  mutexM(),
  handleM(NULL),
  multiM(NULL),
  headerListM(NULL),
  ringBufferM(new cRingBufferLinear(IPTV_BUFFER_SIZE, 7 * TS_SIZE, false, "IPTV CURL")),
  rtspControlM(""),
  modeM(eModeUnknown),
  timeoutM(),
  connectedM(false),
  pausedM(false)
{
  debug1("%s", __PRETTY_FUNCTION__);
  if (ringBufferM) {
     ringBufferM->SetTimeouts(100, 0);
     ringBufferM->SetIoThrottle();
     }
  Connect();
}

cIptvProtocolCurl::~cIptvProtocolCurl()
{
  debug1("%s", __PRETTY_FUNCTION__);
  Disconnect();
  // Free allocated memory
  DELETE_POINTER(ringBufferM);
}

int cIptvProtocolCurl::DebugCallback(CURL *handleP, curl_infotype typeP, char *dataP, size_t sizeP, void *userPtrP)
{
  cIptvProtocolCurl *obj = reinterpret_cast<cIptvProtocolCurl *>(dataP);

  if (obj) {
     switch (typeP) {
       case CURLINFO_TEXT:
            debug8("%s INFO %.*s", __PRETTY_FUNCTION__, (int)sizeP, dataP);
            break;
       case CURLINFO_HEADER_IN:
            debug8("%s HEAD <<< %.*s", __PRETTY_FUNCTION__,  (int)sizeP, dataP);
            break;
       case CURLINFO_HEADER_OUT:
            debug8("%s HEAD >>>\n%.*s", __PRETTY_FUNCTION__, (int)sizeP, dataP);
            break;
       case CURLINFO_DATA_IN:
            debug8("%s DATA <<< %zu", __PRETTY_FUNCTION__,  sizeP);
            break;
       case CURLINFO_DATA_OUT:
            debug8("%s DATA >>> %zu", __PRETTY_FUNCTION__, sizeP);
            break;
       default:
            break;
       }
     }

  return 0;
}

size_t cIptvProtocolCurl::WriteCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP)
{
  cIptvProtocolCurl *obj = reinterpret_cast<cIptvProtocolCurl *>(dataP);
  size_t len = sizeP * nmembP;
  debug16("%s (, %zu, %zu, ) len=%zu", __PRETTY_FUNCTION__, sizeP, nmembP, len);

  if (obj && !obj->PutData((unsigned char *)ptrP, (int)len))
     return CURL_WRITEFUNC_PAUSE;

  return len;
}

size_t cIptvProtocolCurl::WriteRtspCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP)
{
  cIptvProtocolCurl *obj = reinterpret_cast<cIptvProtocolCurl *>(dataP);
  size_t len = sizeP * nmembP;
  unsigned char *p = (unsigned char *)ptrP;
  debug16("%s (, %zu, %zu, ) len=%zu", __PRETTY_FUNCTION__, sizeP, nmembP, len);

  // Validate packet header ('$') and channel (0)
  if (obj && (p[0] == 0x24) && (p[1] == 0)) {
     int length = (p[2] << 8) | p[3];
     if (length > 3) {
        // Skip interleave header
        p += 4;
        // http://tools.ietf.org/html/rfc3550
        // http://tools.ietf.org/html/rfc2250
        // Version
        unsigned int v = (p[0] >> 6) & 0x03;
        // Extension bit
        unsigned int x = (p[0] >> 4) & 0x01;
        // CSCR count
        unsigned int cc = p[0] & 0x0F;
        // Payload type: MPEG2 TS = 33
        //unsigned int pt = p[1] & 0x7F;
        // Header lenght
        unsigned int headerlen = (3 + cc) * (unsigned int)sizeof(uint32_t);
        // Check if extension
        if (x) {
           // Extension header length
           unsigned int ehl = (((p[headerlen + 2] & 0xFF) << 8) |(p[headerlen + 3] & 0xFF));
           // Update header length
           headerlen += (ehl + 1) * (unsigned int)sizeof(uint32_t);
           }
        // Check that rtp is version 2 and payload contains multiple of TS packet data
        if ((v == 2) && (((length - headerlen) % TS_SIZE) == 0) && (p[headerlen] == TS_SYNC_BYTE)) {
           // Set argument point to payload in read buffer
           obj->PutData(&p[headerlen], (length - headerlen));
           }
        }
     }

  return len;
}

size_t cIptvProtocolCurl::DescribeCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP)
{
  cIptvProtocolCurl *obj = reinterpret_cast<cIptvProtocolCurl *>(dataP);
  size_t len = sizeP * nmembP;
  debug16("%s (, %zu, %zu, ) len=%zu", __PRETTY_FUNCTION__, sizeP, nmembP, len);

  bool found = false;
  cString control = "";
  char *p = (char *)ptrP;
  char *r = strtok(p, "\r\n");

  while (r) {
    debug16("%s (, %zu, %zu, ) len=%zu r=%s", __PRETTY_FUNCTION__, sizeP, nmembP, len, r);
    // Look for a media name: "video"
    if (strstr(r, "m=video")) {
       found = true;
       }
    // ... and find out its' attribute
    if (found && strstr(r, "a=control")) {
       char *s = NULL;
       if (sscanf(r, "a=control:%255ms", &s) == 1)
          control = compactspace(s);
       free(s);
       break;
       }
    r = strtok(NULL, "\r\n");
    }

  if (!isempty(*control) && obj)
     obj->SetRtspControl(*control);

  return len;
}

size_t cIptvProtocolCurl::HeaderCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP)
{
  //cIptvProtocolCurl *obj = reinterpret_cast<cIptvProtocolCurl *>(dataP);
  size_t len = sizeP * nmembP;
  debug16("%s (, %zu, %zu, ) len=%zu", __PRETTY_FUNCTION__, sizeP, nmembP, len);

  char *p = (char *)ptrP;
  char *r = strtok(p, "\r\n");

  while (r) {
    debug16("%s (, %zu, %zu, ) len=%zu r=%s", __PRETTY_FUNCTION__, sizeP, nmembP, len, r);
    r = strtok(NULL, "\r\n");
    }

  return len;
}

void cIptvProtocolCurl::SetRtspControl(const char *controlP)
{
  cMutexLock MutexLock(&mutexM);
  debug16("%s (%s)", __PRETTY_FUNCTION__, controlP);
  cString protocol = ChangeCase(controlP, false).Truncate(7);
  if (startswith(*protocol, "rtsp://")) {
     streamUrlM = controlP;
     rtspControlM = "";
     }
  else
     rtspControlM = controlP;
}

bool cIptvProtocolCurl::PutData(unsigned char *dataP, int lenP)
{
  cMutexLock MutexLock(&mutexM);
  debug16("%s (, %d)", __PRETTY_FUNCTION__, lenP);
  if (pausedM)
     return false;
  if (ringBufferM && (lenP >= 0)) {
     // Should we pause the transfer ?
     if (ringBufferM->Free() < (2 * CURL_MAX_WRITE_SIZE)) {
        debug1("%s Pause free=%d available=%d len=%d", __PRETTY_FUNCTION__,
              ringBufferM->Free(), ringBufferM->Available(), lenP);
        pausedM = true;
        return false;
        }
     int p = ringBufferM->Put(dataP, lenP);
     if (p != lenP)
        ringBufferM->ReportOverflow(lenP - p);
     }

  return true;
}

void cIptvProtocolCurl::DelData(int lenP)
{
  cMutexLock MutexLock(&mutexM);
  debug16("%s", __PRETTY_FUNCTION__);
  if (ringBufferM && (lenP >= 0))
     ringBufferM->Del(lenP);
}

void cIptvProtocolCurl::ClearData()
{
  debug16("%s", __PRETTY_FUNCTION__);
  if (ringBufferM)
     ringBufferM->Clear();
}

unsigned char *cIptvProtocolCurl::GetData(int &lenP)
{
  cMutexLock MutexLock(&mutexM);
  debug16("%s", __PRETTY_FUNCTION__);
  unsigned char *p = NULL;
  lenP = 0;
  if (ringBufferM) {
     int count = 0;
     p = ringBufferM->Get(count);
#if 0
     if (p && count >= TS_SIZE) {
        if (*p != TS_SYNC_BYTE) {
           for (int i = 1; i < count; ++i) {
               if (p[i] == TS_SYNC_BYTE) {
                  count = i;
                  break;
                  }
               }
           error("IPTV skipped %d bytes to sync on TS packet", count);
           ringBufferM->Del(count);
           lenP = 0;
           return NULL;
           }
        }
#endif
     count -= (count % TS_SIZE);
     lenP = count;
     }

  return p;
}

bool cIptvProtocolCurl::Connect()
{
  cMutexLock MutexLock(&mutexM);
  debug1("%s", __PRETTY_FUNCTION__);
  if (connectedM)
     return true;

  // Initialize the curl session
  if (!handleM)
     handleM = curl_easy_init();

  if (handleM && !isempty(*streamUrlM)) {
     CURLcode res = CURLE_OK;
     cString netrc = cString::sprintf("%s/netrc", IptvConfig.GetConfigDirectory());

     // Verbose output
     iptv_curl_easy_setopt(handleM, CURLOPT_VERBOSE, 1L);
     iptv_curl_easy_setopt(handleM, CURLOPT_DEBUGFUNCTION, cIptvProtocolCurl::DebugCallback);
     iptv_curl_easy_setopt(handleM, CURLOPT_DEBUGDATA, this);

     // Set callbacks
     iptv_curl_easy_setopt(handleM, CURLOPT_WRITEFUNCTION, cIptvProtocolCurl::WriteCallback);
     iptv_curl_easy_setopt(handleM, CURLOPT_WRITEDATA, this);
     iptv_curl_easy_setopt(handleM, CURLOPT_HEADERFUNCTION, cIptvProtocolCurl::HeaderCallback);
     iptv_curl_easy_setopt(handleM, CURLOPT_WRITEHEADER, this);

     // No progress meter and no signaling
     iptv_curl_easy_setopt(handleM, CURLOPT_NOPROGRESS, 1L);
     iptv_curl_easy_setopt(handleM, CURLOPT_NOSIGNAL, 1L);

     // Support netrc
     iptv_curl_easy_setopt(handleM, CURLOPT_NETRC, (long)CURL_NETRC_OPTIONAL);
     iptv_curl_easy_setopt(handleM, CURLOPT_NETRC_FILE, *netrc);

     // Set timeouts
     iptv_curl_easy_setopt(handleM, CURLOPT_CONNECTTIMEOUT, (long)eConnectTimeoutS);
     iptv_curl_easy_setopt(handleM, CURLOPT_LOW_SPEED_LIMIT, (long)eLowSpeedLimitBytes);
     iptv_curl_easy_setopt(handleM, CURLOPT_LOW_SPEED_TIME, (long)eLowSpeedTimeoutS);

     // Set user-agent
     iptv_curl_easy_setopt(handleM, CURLOPT_USERAGENT, *cString::sprintf("vdr-%s/%s", PLUGIN_NAME_I18N, VERSION));

     // Set URL
     char *p = curl_easy_unescape(handleM, *streamUrlM, 0, NULL);
     streamUrlM = p;
     curl_free(p);
     iptv_curl_easy_setopt(handleM, CURLOPT_URL, *streamUrlM);

     // Protocol specific initializations
     switch (modeM) {
#ifdef USE_RTSP
       case eModeRtsp:
            {
            cString uri, control, transport, range;

            // Create the listening socket for UDP mode
            if (!streamParamM)
               OpenSocket(streamPortM);

            // Request server options
            uri = cString::sprintf("%s", *streamUrlM);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_STREAM_URI, *uri);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_OPTIONS);
            iptv_curl_easy_perform(handleM);

            // Request session description - SDP is delivered in message body and not in the header!
            iptv_curl_easy_setopt(handleM, CURLOPT_WRITEFUNCTION, cIptvProtocolCurl::DescribeCallback);
            iptv_curl_easy_setopt(handleM, CURLOPT_WRITEDATA, this);
            uri = cString::sprintf("%s", *streamUrlM);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_STREAM_URI, *uri);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_DESCRIBE);
            iptv_curl_easy_perform(handleM);
            iptv_curl_easy_setopt(handleM, CURLOPT_WRITEFUNCTION, NULL);
            iptv_curl_easy_setopt(handleM, CURLOPT_WRITEDATA, NULL);

            // Setup media stream
            if (isempty(*rtspControlM))
               uri = cString::sprintf("%s", *streamUrlM);
            else
               uri = cString::sprintf("%s/%s", *streamUrlM, *rtspControlM);
            if (streamParamM)
               transport = "RTP/AVP/TCP;unicast;interleaved=0-1";
            else
               transport = cString::sprintf("RTP/AVP;unicast;client_port=%d-%d", streamPortM, streamPortM + 1);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_STREAM_URI, *uri);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_TRANSPORT, *transport);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_SETUP);
            iptv_curl_easy_perform(handleM);

            // Start playing
            uri = cString::sprintf("%s/", *streamUrlM);
            range = "0.000-";
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_STREAM_URI, *uri);
            //iptv_curl_easy_setopt(handleM, CURLOPT_RANGE, *range);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_PLAY);
            iptv_curl_easy_perform(handleM);

            // Start receiving
            if (streamParamM) {
               iptv_curl_easy_setopt(handleM, CURLOPT_INTERLEAVEFUNCTION, cIptvProtocolCurl::WriteRtspCallback);
               iptv_curl_easy_setopt(handleM, CURLOPT_INTERLEAVEDATA, this);
               iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_RECEIVE);
               iptv_curl_easy_perform(handleM);
               }

            // Don't add handle into multi set
            isActiveM = true;
            }
            break;
#endif
       case eModeHttp:
       case eModeHttps:
            {
            // Limit download speed (bytes/s)
            iptv_curl_easy_setopt(handleM, CURLOPT_MAX_RECV_SPEED_LARGE, eMaxDownloadSpeedMBits * 131072L);

            // Follow location
            iptv_curl_easy_setopt(handleM, CURLOPT_FOLLOWLOCATION, 1L);

            // Fail if HTTP return code is >= 400
            iptv_curl_easy_setopt(handleM, CURLOPT_FAILONERROR, 1L);

            // Set additional headers to prevent caching
            headerListM = curl_slist_append(headerListM, "Cache-Control: no-store, no-cache, must-revalidate");
            headerListM = curl_slist_append(headerListM, "Cache-Control: post-check=0, pre-check=0");
            headerListM = curl_slist_append(headerListM, "Pragma: no-cache");
            headerListM = curl_slist_append(headerListM, "Expires: Mon, 26 Jul 1997 05:00:00 GMT");
            iptv_curl_easy_setopt(handleM, CURLOPT_HTTPHEADER, headerListM);

            // Initialize multi set and add handle into it
            if (!multiM)
               multiM = curl_multi_init();
            if (multiM)
               curl_multi_add_handle(multiM, handleM);
            }
            break;

       case eModeFile:
            {
            // Set timeout
            iptv_curl_easy_setopt(handleM, CURLOPT_TIMEOUT_MS, 10L);

            // Initialize multi set and add handle into it
            if (!multiM)
               multiM = curl_multi_init();
            if (multiM)
               curl_multi_add_handle(multiM, handleM);
            }
            break;

       case eModeUnknown:
       default:
            break;
       }

     timeoutM.Set(eKeepAliveIntervalMs);
     connectedM = true;
     return true;
     }

  return false;
}

bool cIptvProtocolCurl::Disconnect()
{
  cMutexLock MutexLock(&mutexM);
  debug1("%s", __PRETTY_FUNCTION__);
  if (!connectedM)
     return true;

  // Terminate curl session
  if (handleM) {
     // Remove handle from multi set
     if (multiM) {
        curl_multi_remove_handle(multiM, handleM);
        curl_multi_cleanup(multiM);
        multiM = NULL;
        }

     // Mode specific tricks
     switch (modeM) {
#ifdef USE_RTSP
       case eModeRtsp:
            {
            CURLcode res = CURLE_OK;
            // Teardown rtsp session
            cString uri = cString::sprintf("%s/", *streamUrlM);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_STREAM_URI, *uri);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_TEARDOWN);
            iptv_curl_easy_perform(handleM);
            rtspControlM = "";
            isActiveM = false;
            // Close the listening socket
            CloseSocket();
            }
            break;
#endif
       case eModeHttp:
       case eModeHttps:
       case eModeFile:
       case eModeUnknown:
       default:
            break;
       }

     // Cleanup curl stuff
     if (headerListM) {
        curl_slist_free_all(headerListM);
        headerListM = NULL;
        }
     curl_easy_cleanup(handleM);
     handleM = NULL;
     }

  ClearData();
  connectedM = false;
  return true;
}

bool cIptvProtocolCurl::Open(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  return Connect();
}

bool cIptvProtocolCurl::Close(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  Disconnect();
  return true;
}

int cIptvProtocolCurl::Read(unsigned char* bufferAddrP, unsigned int bufferLenP)
{
  debug16("%s (, %u)", __PRETTY_FUNCTION__, bufferLenP);
  int len = 0;
  if (ringBufferM) {
     // Fill up the buffer
     if (handleM) {
        switch (modeM) {
#ifdef USE_RTSP
          case eModeRtsp:
               {
               cMutexLock MutexLock(&mutexM);
               CURLcode res = CURLE_OK;

               // Remember the heart beat
               if (timeoutM.TimedOut()) {
                  debug1("%s KeepAlive", __PRETTY_FUNCTION__);
                  cString uri = cString::sprintf("%s", *streamUrlM);
                  iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_STREAM_URI, *uri);
                  iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_OPTIONS);
                  iptv_curl_easy_perform(handleM);
                  timeoutM.Set(eKeepAliveIntervalMs);
                  }

               // Check whether UDP or TCP mode used
               if (streamParamM) {
                  iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_RECEIVE);
                  iptv_curl_easy_perform(handleM);
                  }
               else
                  return cIptvUdpSocket::Read(bufferAddrP, bufferLenP);
               }
               break;
#endif
          case eModeFile:
          case eModeHttp:
          case eModeHttps:
               if (multiM) {
                  CURLMcode res;
                  int running_handles;

                  do {
                    cMutexLock MutexLock(&mutexM);
                    res = curl_multi_perform(multiM, &running_handles);
                  } while (res == CURLM_CALL_MULTI_PERFORM);

                  // Use 20% threshold before continuing to filling up the buffer.
                  mutexM.Lock();
                  if (pausedM && (ringBufferM->Available() < (IPTV_BUFFER_SIZE / 5))) {
                     debug1("%s Continue free=%d available=%d", __PRETTY_FUNCTION__,
                            ringBufferM->Free(), ringBufferM->Available());
                     pausedM = false;
                     curl_easy_pause(handleM, CURLPAUSE_CONT);
                     }
                  mutexM.Unlock();

                  // Check if end of file
                  if (running_handles == 0) {
                     int msgcount;
                     mutexM.Lock();
                     CURLMsg *msg = curl_multi_info_read(multiM, &msgcount);
                     mutexM.Unlock();
                     if (msg && (msg->msg == CURLMSG_DONE)) {
                        debug1("%s Done %s (%d)", __PRETTY_FUNCTION__,
                               curl_easy_strerror(msg->data.result), msg->data.result);
                        Disconnect();
                        Connect();
                        }
                     }
                  }
               break;

          case eModeUnknown:
          default:
               break;
          }
        }

     // ... and try to empty it
     unsigned char *p = GetData(len);
     if (p && (len > 0)) {
        len = min(len, (int)bufferLenP);
        memcpy(bufferAddrP, p, len);
        DelData(len);
        debug16("%s Get %d bytes", __PRETTY_FUNCTION__, len);
        }
     }

  return len;
}

bool cIptvProtocolCurl::SetSource(const char* locationP, const int parameterP, const int indexP)
{
  debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, locationP, parameterP, indexP);
  if (!isempty(locationP)) {
     // Disconnect
     Disconnect();
     // Update stream URL
     streamUrlM = locationP;
     cString protocol = ChangeCase(streamUrlM, false).Truncate(5);
     if (startswith(*protocol, "rtsp"))
        modeM = eModeRtsp;
     else if (startswith(*protocol, "https"))
        modeM = eModeHttps;
     else if (startswith(*protocol, "http"))
        modeM = eModeHttp;
     else if (startswith(*protocol, "file"))
        modeM = eModeFile;
     else
        modeM = eModeUnknown;
     debug1("%s (%s, %d, %d) protocol=%s mode=%d", __PRETTY_FUNCTION__, locationP, parameterP, indexP, *protocol, modeM);
     // Update stream parameter - force UDP mode for RTSP
     streamParamM = (modeM == eModeRtsp) ? 0 : parameterP;
     // Update listen port
     streamPortM = IptvConfig.GetProtocolBasePort() + indexP * 2;
     // Reconnect
     Connect();
     }
  return true;
}

bool cIptvProtocolCurl::SetPid(int pidP, int typeP, bool onP)
{
  debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);
  return true;
}

cString cIptvProtocolCurl::GetInformation(void)
{
  debug16("%s", __PRETTY_FUNCTION__);
  return cString::sprintf("%s [%d]", *streamUrlM, streamParamM);
}
