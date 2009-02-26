/*
 * protocoludp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_SOCKET_H
#define __IPTV_SOCKET_H

#include <arpa/inet.h>

class cIptvSocket {
protected:
  int socketPort;
  int socketDesc;
  struct sockaddr_in sockAddr;
  bool isActive;

protected:
  bool OpenSocket(const int Port, const bool isUdp);
  void CloseSocket(void);

public:
  cIptvSocket();
  virtual ~cIptvSocket();
};

class cIptvUdpSocket : public cIptvSocket {
public:
  cIptvUdpSocket();
  virtual ~cIptvUdpSocket();
  virtual int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool OpenSocket(const int Port);
};

class cIptvTcpSocket : public cIptvSocket {
public:
  cIptvTcpSocket();
  virtual ~cIptvTcpSocket();
  virtual int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool OpenSocket(const int Port);
};

#endif // __IPTV_SOCKET_H

