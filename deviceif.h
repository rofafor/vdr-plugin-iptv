/*
 * deviceif.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_DEVICEIF_H
#define __IPTV_DEVICEIF_H

class cIptvDeviceIf {
public:
  cIptvDeviceIf() {}
  virtual ~cIptvDeviceIf() {}
  virtual void WriteData(u_char *bufferP, int lengthP) = 0;
  virtual unsigned int CheckData(void) = 0;

private:
  cIptvDeviceIf(const cIptvDeviceIf&);
  cIptvDeviceIf& operator=(const cIptvDeviceIf&);
};

#endif // __IPTV_DEVICEIF_H
