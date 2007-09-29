/*
 * protocoludp.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocoludp.c,v 1.10 2007/09/29 16:21:05 rahrenbe Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>

#include "common.h"
#include "config.h"
#include "protocoludp.h"

cIptvProtocolUdp::cIptvProtocolUdp()
: streamPort(1234),
  socketDesc(-1),
  readBufferLen(TS_SIZE * IptvConfig.GetReadBufferTsCount()),
  isActive(false)
{
  debug("cIptvProtocolUdp::cIptvProtocolUdp()\n");
  streamAddr = strdup("");
  // Allocate receive buffer
  readBuffer = MALLOC(unsigned char, readBufferLen);
  if (!readBuffer)
     error("ERROR: MALLOC() failed in ProtocolUdp()");
}

cIptvProtocolUdp::~cIptvProtocolUdp()
{
  debug("cIptvProtocolUdp::~cIptvProtocolUdp()\n");
  // Drop the multicast group and close the socket
  Close();
  // Free allocated memory
  free(streamAddr);
  free(readBuffer);
}

bool cIptvProtocolUdp::OpenSocket(const int Port)
{
  debug("cIptvProtocolUdp::OpenSocket()\n");
  // If socket is there already and it is bound to a different port, it must
  // be closed first
  if (Port != streamPort) {
     debug("cIptvProtocolUdp::OpenSocket(): Socket tear-down\n");
     CloseSocket();
     }
  // Bind to the socket if it is not active already
  if (socketDesc < 0) {
     int yes = 1;     
     // Create socket
     socketDesc = socket(PF_INET, SOCK_DGRAM, 0);
     if (socketDesc < 0) {
        char tmp[64];
        error("ERROR: socket(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return false;
        }
     // Make it use non-blocking I/O to avoid stuck read calls
     if (fcntl(socketDesc, F_SETFL, O_NONBLOCK)) {
        char tmp[64];
        error("ERROR: fcntl(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }
     // Allow multiple sockets to use the same PORT number
     if (setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &yes,
		    sizeof(yes)) < 0) {
        char tmp[64];
        error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }
     // Bind socket
     memset(&sockAddr, '\0', sizeof(sockAddr));
     sockAddr.sin_family = AF_INET;
     sockAddr.sin_port = htons(Port);
     sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     int err = bind(socketDesc, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
     if (err < 0) {
        char tmp[64];
        error("ERROR: bind(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }
     // Update stream port
     streamPort = Port;
     }
  return true;
}

void cIptvProtocolUdp::CloseSocket(void)
{
  debug("cIptvProtocolUdp::CloseSocket()\n");
  // Check if socket exists
  if (socketDesc >= 0) {
     close(socketDesc);
     socketDesc = -1;
     }
}

bool cIptvProtocolUdp::JoinMulticast(void)
{
  debug("cIptvProtocolUdp::JoinMulticast()\n");
  // Check that stream address is valid
  if (!isActive && !isempty(streamAddr)) {
     // Ensure that socket is valid
     OpenSocket(streamPort);
     // Join a new multicast group
     struct ip_mreq mreq;
     mreq.imr_multiaddr.s_addr = inet_addr(streamAddr);
     mreq.imr_interface.s_addr = htonl(INADDR_ANY);
     int err = setsockopt(socketDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                          sizeof(mreq));
     if (err < 0) {
        char tmp[64];
        error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return false;
        }
     // Update multicasting flag
     isActive = true;
     }
  return true;
}

bool cIptvProtocolUdp::DropMulticast(void)
{
  debug("cIptvProtocolUdp::DropMulticast()\n");
  // Check that stream address is valid
  if (isActive && !isempty(streamAddr)) {
      // Ensure that socket is valid
      OpenSocket(streamPort);
      // Drop the multicast group
      struct ip_mreq mreq;
      mreq.imr_multiaddr.s_addr = inet_addr(streamAddr);
      mreq.imr_interface.s_addr = htonl(INADDR_ANY);
      int err = setsockopt(socketDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,
                           sizeof(mreq));
      if (err < 0) {
         char tmp[64];
         error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
         return false;
         }
      // Update multicasting flag
      isActive = false;
     }
  return true;
}

int cIptvProtocolUdp::Read(unsigned char* *BufferAddr)
{
  //debug("cIptvProtocolUdp::Read()\n");
  socklen_t addrlen = sizeof(sockAddr);
  // Set argument point to read buffer
  *BufferAddr = readBuffer;
  // Wait for data
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  // Use select
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(socketDesc, &rfds);
  int retval = select(socketDesc + 1, &rfds, NULL, NULL, &tv);
  // Check if error
  if (retval < 0) {
     char tmp[64];
     error("ERROR: select(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     return -1;
     }
  // Check if data available
  else if (retval) {
     // Read data from socket
     int len = recvfrom(socketDesc, readBuffer, readBufferLen, MSG_DONTWAIT,
                        (struct sockaddr *)&sockAddr, &addrlen);
     if ((len > 0) && (readBuffer[0] == 0x47)) {
        // Set argument point to read buffer
        *BufferAddr = &readBuffer[0];
        return len;
        }
     else if (len > 3) {
        // http://www.networksorcery.com/enp/rfc/rfc2250.txt
        // version
        unsigned int v = (readBuffer[0] >> 6) & 0x03;
        // extension bit
        unsigned int x = (readBuffer[0] >> 4) & 0x01;
        // cscr count
        unsigned int cc = readBuffer[0] & 0x0F;
        // payload type
        unsigned int pt = readBuffer[1] & 0x7F;
        // header lenght
        unsigned int headerlen = (3 + cc) * sizeof(uint32_t);
        // check if extension
        if (x) {
           // extension header length
           unsigned int ehl = (((readBuffer[headerlen + 2] & 0xFF) << 8) | (readBuffer[headerlen + 3] & 0xFF));
           // update header length
           headerlen += (ehl + 1) * sizeof(uint32_t);
           }
        // Check that rtp is version 2, payload type is MPEG2 TS
        // and payload contains multiple of TS packet data
        if ((v == 2) && (pt == 33) && (((len - headerlen) % TS_SIZE) == 0)) {
           // Set argument point to payload in read buffer
           *BufferAddr = &readBuffer[headerlen];
           return (len - headerlen);
           }
        }
     }
  return 0;
}

bool cIptvProtocolUdp::Open(void)
{
  debug("cIptvProtocolUdp::Open(): streamAddr=%s\n", streamAddr);
  // Join a new multicast group
  JoinMulticast();
  return true;
}

bool cIptvProtocolUdp::Close(void)
{
  debug("cIptvProtocolUdp::Close(): streamAddr=%s\n", streamAddr);
  // Drop the multicast group
  DropMulticast();
  // Close the socket
  CloseSocket();
  return true;
}

bool cIptvProtocolUdp::Set(const char* Address, const int Port)
{
  debug("cIptvProtocolUdp::Set(): %s:%d\n", Address, Port);
  if (!isempty(Address)) {
    // Drop the multicast group
    DropMulticast();
    // Update stream address and port
    streamAddr = strcpyrealloc(streamAddr, Address);
    streamPort = Port;
    // Join a new multicast group
    JoinMulticast();
    }
  return true;
}
