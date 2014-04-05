/*
 * socket.h: IPTV plugin for the Video Disk Recorder
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
  int socketPortM;

protected:
  enum {
    eReportIntervalS = 300 // in seconds
  };
  int socketDescM;
  struct sockaddr_in sockAddrM;
  time_t lastErrorReportM;
  int packetErrorsM;
  int sequenceNumberM;
  bool isActiveM;

protected:
  bool OpenSocket(const int portP, const bool isUdpP);
  void CloseSocket(void);
  bool CheckAddress(const char *addrP, in_addr_t *inAddrP);

public:
  cIptvSocket();
  virtual ~cIptvSocket();
};

class cIptvUdpSocket : public cIptvSocket {
private:
  in_addr_t streamAddrM;
  in_addr_t sourceAddrM;
  bool useIGMPv3M;

public:
  cIptvUdpSocket();
  virtual ~cIptvUdpSocket();
  virtual int Read(unsigned char* bufferAddrP, unsigned int bufferLenP);
  bool OpenSocket(const int Port);
  bool OpenSocket(const int Port, const char *streamAddrP, const char *sourceAddrP, bool useIGMPv3P);
  void CloseSocket(void);
  bool JoinMulticast(void);
  bool DropMulticast(void);
};

class cIptvTcpSocket : public cIptvSocket {
public:
  cIptvTcpSocket();
  virtual ~cIptvTcpSocket();
  virtual int Read(unsigned char* bufferAddrP, unsigned int bufferLenP);
  bool OpenSocket(const int portP, const char *streamAddrP);
  void CloseSocket(void);
  bool ConnectSocket(void);
  bool ReadChar(char* bufferAddrP, unsigned int timeoutMsP);
  bool Write(const char* bufferAddrP, unsigned int bufferLenP);
};

#endif // __IPTV_SOCKET_H

