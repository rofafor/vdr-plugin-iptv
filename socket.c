/*
 * socket.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: socket.c,v 1.1 2007/10/21 13:31:21 ajhseppa Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>

#include "common.h"
#include "config.h"
#include "socket.h"

cIptvSocket::cIptvSocket()
: socketPort(IptvConfig.GetExtProtocolBasePort()),
  socketDesc(-1),
  readBufferLen(TS_SIZE * IptvConfig.GetReadBufferTsCount()),
  isActive(false)
{
  debug("cIptvSocket::cIptvSocket()\n");
  // Allocate receive buffer
  readBuffer = MALLOC(unsigned char, readBufferLen);
  if (!readBuffer)
     error("ERROR: MALLOC() failed in socket()");
}

cIptvSocket::~cIptvSocket()
{
  debug("cIptvSocket::~cIptvSocket()\n");
  // Close the socket
  CloseSocket();
  // Free allocated memory
  free(readBuffer);
}

bool cIptvSocket::OpenSocket(const int Port, const bool isUdp)
{
  debug("cIptvSocket::OpenSocket()\n");
  // If socket is there already and it is bound to a different port, it must
  // be closed first
  if (Port != socketPort) {
     debug("cIptvSocket::OpenSocket(): Socket tear-down\n");
     CloseSocket();
     }
  // Bind to the socket if it is not active already
  if (socketDesc < 0) {
     int yes = 1;     
     // Create socket
     if (isUdp)
        socketDesc = socket(PF_INET, SOCK_DGRAM, 0);
     else
        socketDesc = socket(PF_INET, SOCK_STREAM, 0);
     ERROR_IF_RET(socketDesc < 0, "socket()", return false);
     // Make it use non-blocking I/O to avoid stuck read calls
     ERROR_IF_FUNC(fcntl(socketDesc, F_SETFL, O_NONBLOCK), "fcntl()", CloseSocket(), return false);
     // Allow multiple sockets to use the same PORT number
     ERROR_IF_FUNC(setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &yes,
                              sizeof(yes)) < 0, "setsockopt()", CloseSocket(), return false);
     // Bind socket
     memset(&sockAddr, '\0', sizeof(sockAddr));
     sockAddr.sin_family = AF_INET;
     sockAddr.sin_port = htons(Port);
     sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     if (isUdp) {
        int err = bind(socketDesc, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
        ERROR_IF_FUNC(err < 0, "bind()", CloseSocket(), return false);
     }
     // Update socket port
     socketPort = Port;
     }
  return true;
}

void cIptvSocket::CloseSocket(void)
{
  debug("cIptvSocket::CloseSocket()\n");
  // Check if socket exists
  if (socketDesc >= 0) {
     close(socketDesc);
     socketDesc = -1;
     }
}

int cIptvSocket::ReadUdpSocket(unsigned char* *BufferAddr)
{
  //debug("cIptvSocket::Read()\n");
  // Error out if socket not initialized
  if (socketDesc <= 0) {
    error("ERROR: Invalid socket in %s\n", __FUNCTION__);
    return -1;
  }
  socklen_t addrlen = sizeof(sockAddr);
  // Set argument point to read buffer
  *BufferAddr = readBuffer;
  // Wait for data
  int retval = select_single_desc(socketDesc, 500000, false);
  // Check if error
  if (retval < 0)
     return retval;
  // Check if data available
  else if (retval) {
     int len = 0;
     // Read data from socket
     if (isActive)
        len = recvfrom(socketDesc, readBuffer, readBufferLen, MSG_DONTWAIT,
                       (struct sockaddr *)&sockAddr, &addrlen);
     ERROR_IF_RET(len < 0, "recvfrom()", return len);
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
