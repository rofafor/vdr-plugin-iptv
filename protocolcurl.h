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

class cIptvProtocolCurl : public cIptvProtocolIf {
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
    eConnectTimeoutS       = 5,  // in seconds
    eSelectTimeoutMs       = 10, // in milliseconds
    eMaxDownloadSpeedMBits = 20  // in megabits per second
  };

  static size_t WriteCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP);
  static size_t WriteRtspCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP);
  static size_t DescribeCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP);
  static size_t HeaderCallback(void *ptrP, size_t sizeP, size_t nmembP, void *dataP);

  cString streamUrlM;
  int streamParamM;
  cMutex mutexM;
  CURL *handleM;
  CURLM *multiM;
  struct curl_slist *headerListM;
  cRingBufferLinear *ringBufferM;
  cString rtspControlM;
  eModeType modeM;
  bool connectedM;
  bool pausedM;

  bool Connect(void);
  bool Disconnect(void);
  bool PutData(unsigned char *dataP, int lenP);
  void DelData(int lenP);
  void ClearData(void);
  unsigned char *GetData(unsigned int *lenP);

public:
  cIptvProtocolCurl();
  virtual ~cIptvProtocolCurl();
  int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool Set(const char* Location, const int Parameter, const int Index);
  bool Open(void);
  bool Close(void);
  cString GetInformation(void);
  void SetRtspControl(const char *controlP);
};

#endif // __IPTV_PROTOCOLCURL_H

