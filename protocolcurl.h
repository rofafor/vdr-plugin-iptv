/*
 * protocolcurl.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_PROTOCOLCURL_H
#define __IPTV_PROTOCOLCURL_H

#include <curl/curl.h>
#include <curl/easy.h>

#include <vdr/ringbuffer.h>
#include <vdr/thread.h>
#include <vdr/tools.h>

#include "protocolif.h"
#include "socket.h"

class cIptvProtocolCurl : public cIptvUdpSocket, public cIptvProtocolIf {
private:
  enum eModeType {
    eModeUnknown = 0,
    eModeHttp,
    eModeHttps,
    eModeRtsp,
    eModeFile,
    eModeCount
  };
  enum {
    eConnectTimeoutS       = 5,     // in seconds
    eLowSpeedTimeoutS      = 3,     // in seconds
    eLowSpeedLimitBytes    = 100,   // in bytes per second
    eMaxDownloadSpeedMBits = 20,    // in megabits per second
    eKeepAliveIntervalMs   = 300000 // in milliseconds
  };

  static int DebugCallback(CURL *handleP, curl_infotype typeP, char *dataP, size_t sizeP, void *userPtrP);
  static size_t WriteCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP);
  static size_t WriteRtspCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP);
  static size_t DescribeCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP);
  static size_t HeaderCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP);

  cString streamUrlM;
  int streamParamM;
  int streamPortM;
  cMutex mutexM;
  CURL *handleM;
  CURLM *multiM;
  struct curl_slist *headerListM;
  cRingBufferLinear *ringBufferM;
  cString rtspControlM;
  eModeType modeM;
  cTimeMs timeoutM;
  bool connectedM;
  bool pausedM;

  bool Connect(void);
  bool Disconnect(void);
  bool PutData(unsigned char *dataP, int lenP);
  void DelData(int lenP);
  void ClearData(void);
  unsigned char *GetData(int &lenP);

public:
  cIptvProtocolCurl();
  virtual ~cIptvProtocolCurl();
  int Read(unsigned char* bufferAddrP, unsigned int bufferLenP);
  bool SetSource(const char* locationP, const int parameterP, const int indexP);
  bool SetPid(int pidP, int typeP, bool onP);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
  void SetRtspControl(const char *controlP);
};

#endif // __IPTV_PROTOCOLCURL_H

