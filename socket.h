/*
 * protocoludp.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_SOCKET_H
#define __IPTV_SOCKET_H

#include <arpa/inet.h>
#ifdef __FreeBSD__
#include <netinet/in.h>
#endif // __FreeBSD__

class cIptvSocket {
private:
  int socketPort;

protected:
  int socketDesc;
  struct sockaddr_in sockAddr;
  bool isActive;

protected:
  bool OpenSocket(const int Port, const bool isUdp);
  void CloseSocket(void);
  bool CheckAddress(const char *Addr, in_addr_t *InAddr);

public:
  cIptvSocket();
  virtual ~cIptvSocket();
};

class cIptvUdpSocket : public cIptvSocket {
private:
  in_addr_t streamAddr;
  in_addr_t sourceAddr;
  bool useIGMPv3;

public:
  cIptvUdpSocket();
  virtual ~cIptvUdpSocket();
  virtual int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool OpenSocket(const int Port);
  bool OpenSocket(const int Port, const char *StreamAddr, const char *SourceAddr, bool UseIGMPv3);
  void CloseSocket(void);
  bool JoinMulticast(void);
  bool DropMulticast(void);
};

class cIptvTcpSocket : public cIptvSocket {
public:
  cIptvTcpSocket();
  virtual ~cIptvTcpSocket();
  virtual int Read(unsigned char* BufferAddr, unsigned int BufferLen);
  bool OpenSocket(const int Port, const char *StreamAddr);
  void CloseSocket(void);
  bool ConnectSocket(void);
  bool ReadChar(char* BufferAddr, unsigned int TimeoutMs);
  bool Write(const char* BufferAddr, unsigned int BufferLen);
};

#endif // __IPTV_SOCKET_H

