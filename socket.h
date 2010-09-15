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
  int socketDesc;
  int socketPort;
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
private:
  in_addr_t sourceAddr;

public:
  cIptvUdpSocket();
  virtual ~cIptvUdpSocket();
  virtual int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool OpenSocket(const int Port, const in_addr_t SourceAddr = INADDR_ANY);
  void CloseSocket(void);
  bool JoinMulticast(const in_addr_t StreamAddr);
  bool DropMulticast(const in_addr_t StreamAddr);
};

class cIptvTcpSocket : public cIptvSocket {
public:
  cIptvTcpSocket();
  virtual ~cIptvTcpSocket();
  virtual int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool OpenSocket(const int Port);
};

#endif // __IPTV_SOCKET_H

