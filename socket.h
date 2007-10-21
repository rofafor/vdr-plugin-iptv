/*
 * protocoludp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: socket.h,v 1.1 2007/10/21 13:31:21 ajhseppa Exp $
 */

#ifndef __IPTV_SOCKET_H
#define __IPTV_SOCKET_H

#include <arpa/inet.h>

class cIptvSocket {
protected:
  int socketPort;
  int socketDesc;
  unsigned char* readBuffer;
  unsigned int readBufferLen;
  struct sockaddr_in sockAddr;
  bool isActive;

protected:
  bool OpenSocket(const int Port, const bool isUdp);
  void CloseSocket(void);
  int ReadUdpSocket(unsigned char* *BufferAddr);

public:
  cIptvSocket();
  virtual ~cIptvSocket();
};

#endif // __IPTV_SOCKET_H

