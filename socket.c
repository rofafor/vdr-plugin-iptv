/*
 * socket.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
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
: socketDesc(-1),
  socketPort(0),
  inetAddr(INADDR_NONE),
  isActive(false)
{
  debug("cIptvSocket::cIptvSocket()\n");
  memset(&sockAddr, 0, sizeof(sockAddr));
}

cIptvSocket::~cIptvSocket()
{
  debug("cIptvSocket::~cIptvSocket()\n");
  // Close the socket
  CloseSocket();
}

bool cIptvSocket::OpenSocket(const in_addr_t InetAddr, const int Port, const bool isUdp)
{
  debug("cIptvSocket::OpenSocket()\n");
  // If socket is there already and it is bound to a different port, it must
  // be closed first
  if (Port != socketPort) {
     debug("cIptvSocket::OpenSocket(): Socket tear-down\n");
     CloseSocket();
     }
  // inetAddr must be set after CloseSocket()
  if (inetAddr != InetAddr)
     inetAddr = InetAddr;
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
     ERROR_IF_FUNC(fcntl(socketDesc, F_SETFL, O_NONBLOCK), "fcntl()",
                   CloseSocket(), return false);
     // Allow multiple sockets to use the same PORT number
     ERROR_IF_FUNC(setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0, "setsockopt(SO_REUSEADDR)",
                   CloseSocket(), return false);
     // Allow packet information to be fetched
     ERROR_IF_FUNC(setsockopt(socketDesc, SOL_IP, IP_PKTINFO, &yes, sizeof(yes)) < 0, "setsockopt(IP_PKTINFO)",
                   CloseSocket(), return false);
     // Bind socket
     memset(&sockAddr, 0, sizeof(sockAddr));
     sockAddr.sin_family = AF_INET;
     sockAddr.sin_port = htons((uint16_t)(Port & 0xFFFF));
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
     socketPort = 0;
     inetAddr = INADDR_NONE;
     memset(&sockAddr, 0, sizeof(sockAddr));
     }
}

// UDP socket class
cIptvUdpSocket::cIptvUdpSocket()
{
  debug("cIptvUdpSocket::cIptvUdpSocket()\n");
}

cIptvUdpSocket::~cIptvUdpSocket()
{
  debug("cIptvUdpSocket::~cIptvUdpSocket()\n");
}

bool cIptvUdpSocket::OpenSocket(const in_addr_t InetAddr, const int Port)
{
  debug("cIptvUdpSocket::OpenSocket()\n");
  return cIptvSocket::OpenSocket(InetAddr, Port, true);
}

int cIptvUdpSocket::Read(unsigned char* BufferAddr, unsigned int BufferLen)
{
  //debug("cIptvUdpSocket::Read()\n");
  // Error out if socket not initialized
  if (socketDesc <= 0) {
     error("Invalid socket in %s\n", __FUNCTION__);
     return -1;
     }
  socklen_t addrlen = sizeof(sockAddr);
  int len = 0;
  struct msghdr msgh;
  struct cmsghdr *cmsg;
  struct iovec iov;
  char cbuf[256];
  // Initialize iov and msgh structures
  memset(&msgh, 0, sizeof(struct msghdr));
  iov.iov_base = BufferAddr;
  iov.iov_len = BufferLen;
  msgh.msg_control = cbuf;
  msgh.msg_controllen = sizeof(cbuf);
  msgh.msg_name = &sockAddr;
  msgh.msg_namelen = addrlen;
  msgh.msg_iov  = &iov;
  msgh.msg_iovlen = 1;
  msgh.msg_flags = 0;
  // Read data from socket
  if (isActive && socketDesc && BufferAddr && (BufferLen > 0))
     len = (int)recvmsg(socketDesc, &msgh, MSG_DONTWAIT);
  if (len < 0)
     return -1;
  else if (len > 0) {
     // Process auxiliary received data and validate source address
     for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
         if ((cmsg->cmsg_level == SOL_IP) && (cmsg->cmsg_type == IP_PKTINFO)) {
            struct in_pktinfo *i = (struct in_pktinfo *)CMSG_DATA(cmsg);
            if (i->ipi_addr.s_addr != inetAddr)
               return 0;
            }
         }
     if (BufferAddr[0] == TS_SYNC_BYTE)
        return len;
     else if (len > 3) {
        // http://www.networksorcery.com/enp/rfc/rfc2250.txt
        // version
        unsigned int v = (BufferAddr[0] >> 6) & 0x03;
        // extension bit
        unsigned int x = (BufferAddr[0] >> 4) & 0x01;
        // cscr count
        unsigned int cc = BufferAddr[0] & 0x0F;
        // payload type: MPEG2 TS = 33
        //unsigned int pt = readBuffer[1] & 0x7F;
        // header lenght
        unsigned int headerlen = (3 + cc) * (unsigned int)sizeof(uint32_t);
        // check if extension
        if (x) {
           // extension header length
           unsigned int ehl = (((BufferAddr[headerlen + 2] & 0xFF) << 8) |
                               (BufferAddr[headerlen + 3] & 0xFF));
           // update header length
           headerlen += (ehl + 1) * (unsigned int)sizeof(uint32_t);
           }
        // Check that rtp is version 2 and payload contains multiple of TS packet data
        if ((v == 2) && (((len - headerlen) % TS_SIZE) == 0) &&
            (BufferAddr[headerlen] == TS_SYNC_BYTE)) {
           // Set argument point to payload in read buffer
           memmove(BufferAddr, &BufferAddr[headerlen], (len - headerlen));
           return (len - headerlen);
           }
        }
     }
  return 0;
}

// TCP socket class
cIptvTcpSocket::cIptvTcpSocket()
{
  debug("cIptvTcpSocket::cIptvTcpSocket()\n");
}

cIptvTcpSocket::~cIptvTcpSocket()
{
  debug("cIptvTcpSocket::~cIptvTcpSocket()\n");
}

bool cIptvTcpSocket::OpenSocket(const in_addr_t InetAddr, const int Port)
{
  debug("cIptvTcpSocket::OpenSocket()\n");
  return cIptvSocket::OpenSocket(InetAddr, Port, false);
}

int cIptvTcpSocket::Read(unsigned char* BufferAddr, unsigned int BufferLen)
{
  //debug("cIptvTcpSocket::Read()\n");
  // Error out if socket not initialized
  if (socketDesc <= 0) {
     error("Invalid socket in %s\n", __FUNCTION__);
     return -1;
     }
  int len = 0;
  socklen_t addrlen = sizeof(sockAddr);
  // Read data from socket
  if (isActive && socketDesc && BufferAddr && (BufferLen > 0))
     len = (int)recvfrom(socketDesc, BufferAddr, BufferLen, MSG_DONTWAIT,
                         (struct sockaddr *)&sockAddr, &addrlen);
  //if (inetAddr != sockAddr.sin_addr.s_addr)
  //   return -1;
  return len;
}
