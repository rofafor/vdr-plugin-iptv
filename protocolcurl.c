/*
 * protocolcurl.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "config.h"
#include "protocolcurl.h"

#define iptv_curl_easy_setopt(X, Y, Z) \
  if ((res = curl_easy_setopt((X), (Y), (Z))) != CURLE_OK) { \
     error("curl_easy_setopt(%s, %s, %s) failed: %d\n", #X, #Y, #Z, res); \
     }

#define iptv_curl_easy_perform(X) \
  if ((res = curl_easy_perform((X))) != CURLE_OK) { \
     error("curl_easy_perform(%s) failed: %d\n", #X, res); \
     }

cIptvProtocolCurl::cIptvProtocolCurl()
: streamUrlM(""),
  streamParamM(0),
  mutexM(),
  handleM(NULL),
  multiM(NULL),
  headerListM(NULL),
  ringBufferM(new cRingBufferLinear(MEGABYTE(IptvConfig.GetTsBufferSize()), 7 * TS_SIZE,
                                    false, *cString::sprintf("IPTV CURL"))),
  rtspControlM(),
  modeM(eModeUnknown),
  connectedM(false),
  pausedM(false)
{
  debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  if (ringBufferM)
     ringBufferM->SetTimeouts(100, 0);
  Connect();
}

cIptvProtocolCurl::~cIptvProtocolCurl()
{
  debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  Disconnect();
  // Free allocated memory
  DELETE_POINTER(ringBufferM);
}

size_t cIptvProtocolCurl::WriteCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP)
{
  cIptvProtocolCurl *obj = reinterpret_cast<cIptvProtocolCurl *>(dataP);
  size_t len = sizeP * nmembP;
  //debug("cIptvProtocolCurl::%s(%zu)", __FUNCTION__, len);

  if (obj && !obj->PutData((unsigned char *)ptrP, (int)len))
     return CURL_WRITEFUNC_PAUSE;

  return len;
}

size_t cIptvProtocolCurl::WriteRtspCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP)
{
  cIptvProtocolCurl *obj = reinterpret_cast<cIptvProtocolCurl *>(dataP);
  size_t len = sizeP * nmembP;
  unsigned char *p = (unsigned char *)ptrP;
  //debug("cIptvProtocolCurl::%s(%zu)", __FUNCTION__, len);
  
  // Validate packet header ('$') and channel (0)
  if (obj && (p[0] == 0x24 ) && (p[1] == 0)) {
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
  //debug("cIptvProtocolCurl::%s(%zu)", __FUNCTION__, len);

  cString control = "";
  char *p = (char *)ptrP;
  char *r = strtok(p, "\r\n");

  while (r) {
    //debug("cIptvProtocolCurl::%s(%zu): %s", __FUNCTION__, len, r);
    if (strstr(r, "a=control")) {
       char *s = NULL;
       if (sscanf(r, "a=control:%64ms", &s) == 1)
          control = compactspace(s);
       free(s);
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
  //debug("cIptvProtocolCurl::%s(%zu)", __FUNCTION__, len);

  char *p = (char *)ptrP;
  char *r = strtok(p, "\r\n");

  while (r) {
    //debug("cIptvProtocolCurl::%s(%zu): %s", __FUNCTION__, len, r);
    r = strtok(NULL, "\r\n");
    }

  return len;
}

void cIptvProtocolCurl::SetRtspControl(const char *controlP)
{
  cMutexLock MutexLock(&mutexM);
  debug("cIptvProtocolCurl::%s(%s)", __FUNCTION__, controlP);
  rtspControlM = controlP;
}

bool cIptvProtocolCurl::PutData(unsigned char *dataP, int lenP)
{
  cMutexLock MutexLock(&mutexM);
  //debug("cIptvProtocolCurl::%s(%d)", __FUNCTION__, lenP);
  if (pausedM)
     return false;
  if (ringBufferM && (lenP >= 0)) {
     // Should we pause the transfer ?
     if (ringBufferM->Free() < (2 * CURL_MAX_WRITE_SIZE)) {
        debug("cIptvProtocolCurl::%s(pause): free=%d available=%d len=%d", __FUNCTION__,
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
  //debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  if (ringBufferM && (lenP >= 0))
     ringBufferM->Del(lenP);
}

void cIptvProtocolCurl::ClearData()
{
  //debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  if (ringBufferM)
     ringBufferM->Clear();
}

unsigned char *cIptvProtocolCurl::GetData(unsigned int *lenP)
{
  cMutexLock MutexLock(&mutexM);
  //debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  unsigned char *p = NULL;
  *lenP = 0;
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
           error("IPTV skipped %d bytes to sync on TS packet\n", count);
           ringBufferM->Del(count);
           *lenP = 0;
           return NULL;
           }
        }
#endif
     count -= (count % TS_SIZE);
     *lenP = count;
     }

  return p;
}

bool cIptvProtocolCurl::Connect()
{
  cMutexLock MutexLock(&mutexM);
  debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  if (connectedM)
     return true;

  // Initialize the curl session
  if (!handleM)
     handleM = curl_easy_init();
  if (!multiM)
     multiM = curl_multi_init();

  if (handleM && multiM && !isempty(*streamUrlM)) {
     CURLcode res = CURLE_OK;
     cString netrc = cString::sprintf("%s/netrc", IptvConfig.GetConfigDirectory());

#ifdef DEBUG
     // Verbose output
     iptv_curl_easy_setopt(handleM, CURLOPT_VERBOSE, 1L);
#endif

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

     // Set timeout
     iptv_curl_easy_setopt(handleM, CURLOPT_CONNECTTIMEOUT, (long)eConnectTimeoutS);

     // Set user-agent
     iptv_curl_easy_setopt(handleM, CURLOPT_USERAGENT, *cString::sprintf("vdr-%s/%s", PLUGIN_NAME_I18N, VERSION));

     // Set URL
     char *p = curl_easy_unescape(handleM, *streamUrlM, 0, NULL);
     streamUrlM = p;
     curl_free(p);
     iptv_curl_easy_setopt(handleM, CURLOPT_URL, *streamUrlM);

     // Protocol specific initializations
     switch (modeM) {
       case eModeRtsp:
            {
            cString uri, control, transport, range;

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
            uri = cString::sprintf("%s/%s", *streamUrlM, *rtspControlM);
            transport = "RTP/AVP/TCP;unicast;interleaved=0-1";
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_STREAM_URI, *uri);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_TRANSPORT, *transport);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_SETUP);
            iptv_curl_easy_perform(handleM);

            // Start playing
            uri = cString::sprintf("%s/", *streamUrlM);
            range = "0.000-";
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_STREAM_URI, *uri);
            iptv_curl_easy_setopt(handleM, CURLOPT_RANGE, *range);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_PLAY);
            iptv_curl_easy_perform(handleM);

            // Start receiving
            iptv_curl_easy_setopt(handleM, CURLOPT_INTERLEAVEFUNCTION, cIptvProtocolCurl::WriteRtspCallback);
            iptv_curl_easy_setopt(handleM, CURLOPT_INTERLEAVEDATA, this);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_RECEIVE);
            iptv_curl_easy_perform(handleM);
            }
            break;

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
            }
            break;

       case eModeFile:
       case eModeUnknown:
       default:
            break;
       }

     // Add handle into multi set
     curl_multi_add_handle(multiM, handleM);

     connectedM = true;
     return true;
     }

  return false;
}

bool cIptvProtocolCurl::Disconnect()
{
  cMutexLock MutexLock(&mutexM);
  debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  if (!connectedM)
     return true;

  // Terminate curl session
  if (handleM && multiM) {
     // Mode specific tricks
     switch (modeM) {
       case eModeRtsp:
            {
            CURLcode res = CURLE_OK;
            // Teardown rtsp session
            cString uri = cString::sprintf("%s/", *streamUrlM);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_STREAM_URI, *uri);
            iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_TEARDOWN);
            iptv_curl_easy_perform(handleM);
            rtspControlM = "";
            }
            break;

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
     curl_multi_remove_handle(multiM, handleM);
     curl_easy_cleanup(handleM);
     handleM = NULL;
     curl_multi_cleanup(multiM);
     multiM = NULL;
     }

  ClearData();
  connectedM = false;
  return true;
}

bool cIptvProtocolCurl::Open(void)
{
  debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  return Connect();
}

bool cIptvProtocolCurl::Close(void)
{
  debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  Disconnect();
  return true;
}

int cIptvProtocolCurl::Read(unsigned char* bufferAddrP, unsigned int bufferLenP)
{
  //debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  int len = 0;
  if (ringBufferM) {
     // Fill up the buffer
     mutexM.Lock();
     if (handleM && multiM) {
        switch (modeM) {
          case eModeRtsp:
               {
               CURLcode res = CURLE_OK;
               iptv_curl_easy_setopt(handleM, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_RECEIVE);
               iptv_curl_easy_perform(handleM);
               // @todo - How to detect eof?
               }
               break;

          case eModeFile:
          case eModeHttp:
          case eModeHttps:
               {
               CURLMcode res;
               int running_handles;

               do {
                 res = curl_multi_perform(multiM, &running_handles);
               } while (res == CURLM_CALL_MULTI_PERFORM);

               // Shall we continue filling up the buffer?
               if (pausedM && (ringBufferM->Free() > ringBufferM->Available())) {
                  debug("cIptvProtocolCurl::%s(continue): free=%d available=%d", __FUNCTION__,
                        ringBufferM->Free(), ringBufferM->Available());
                  pausedM = false;
                  curl_easy_pause(handleM, CURLPAUSE_CONT);
                  }

               // Check if end of file
               if (running_handles == 0) {
                  int msgcount;
                  CURLMsg *msg = curl_multi_info_read(multiM, &msgcount);
                  if (msg && (msg->msg == CURLMSG_DONE)) {
                     debug("cIptvProtocolCurl::%s(done): %s (%d)", __FUNCTION__,
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
     mutexM.Unlock();

     // ... and try to empty it
     unsigned char *p = GetData(&bufferLenP);
     if (p && (bufferLenP > 0)) {
        memcpy(bufferAddrP, p, bufferLenP);
        DelData(bufferLenP);
        len = bufferLenP;
        //debug("cIptvProtocolCurl::%s(): get %d bytes", __FUNCTION__, len);
        }
     }

  return len;
}

bool cIptvProtocolCurl::Set(const char* locationP, const int parameterP, const int indexP)
{
  debug("cIptvProtocolCurl::%s(%s, %d, %d)", __FUNCTION__, locationP, parameterP, indexP);
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
     debug("cIptvProtocolCurl::%s(): %s (%d)", __FUNCTION__, *protocol, modeM);
     // Update stream parameter
     streamParamM = parameterP;
     // Reconnect
     Connect();
     }
  return true;
}

cString cIptvProtocolCurl::GetInformation(void)
{
  //debug("cIptvProtocolCurl::%s()", __FUNCTION__);
  return cString::sprintf("%s [%d]", *streamUrlM, streamParamM);
}
