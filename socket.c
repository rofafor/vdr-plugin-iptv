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
: socketPort(0),
  socketDesc(-1),
  isActive(false)
{
  debug("cIptvSocket::cIptvSocket()\n");
  memset(&sockAddr, '\0', sizeof(sockAddr));
}

cIptvSocket::~cIptvSocket()
{
  debug("cIptvSocket::~cIptvSocket()\n");
  // Close the socket
  CloseSocket();
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
     ERROR_IF_FUNC(fcntl(socketDesc, F_SETFL, O_NONBLOCK), "fcntl(O_NONBLOCK)",
                   CloseSocket(), return false);
     // Allow multiple sockets to use the same PORT number
     ERROR_IF_FUNC(setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0, "setsockopt(SO_REUSEADDR)",
                   CloseSocket(), return false);
     // Allow packet information to be fetched
     ERROR_IF_FUNC(setsockopt(socketDesc, SOL_IP, IP_PKTINFO, &yes, sizeof(yes)) < 0, "setsockopt(IP_PKTINFO)",
                   CloseSocket(), return false);
     // Bind socket
     memset(&sockAddr, '\0', sizeof(sockAddr));
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
     memset(&sockAddr, 0, sizeof(sockAddr));
     }
}

// UDP socket class
cIptvUdpSocket::cIptvUdpSocket()
: streamAddr(INADDR_ANY)
{
  debug("cIptvUdpSocket::cIptvUdpSocket()\n");
}

cIptvUdpSocket::~cIptvUdpSocket()
{
  debug("cIptvUdpSocket::~cIptvUdpSocket()\n");
}

bool cIptvUdpSocket::OpenSocket(const int Port, const in_addr_t StreamAddr)
{
  debug("cIptvUdpSocket::OpenSocket()\n");
  streamAddr = StreamAddr;
  return cIptvSocket::OpenSocket(Port, true);
}

void cIptvUdpSocket::CloseSocket(void)
{
  debug("cIptvUdpSocket::CloseSocket()\n");
  streamAddr = INADDR_ANY;
  cIptvSocket::CloseSocket();
}

bool cIptvUdpSocket::JoinMulticast(void)
{
  debug("cIptvUdpSocket::JoinMulticast()\n");
  // Check if socket exists
  if (!isActive && (socketDesc >= 0)) {
     // Join a new multicast group
     struct ip_mreq mreq;
     mreq.imr_multiaddr.s_addr = streamAddr;
     mreq.imr_interface.s_addr = htonl(INADDR_ANY);
     ERROR_IF_RET(setsockopt(socketDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0, "setsockopt(IP_ADD_MEMBERSHIP)", return false);
     // Update multicasting flag
     isActive = true;
     }
  return true;
}

bool cIptvUdpSocket::DropMulticast(void)
{
  debug("cIptvUdpSocket::DropMulticast()\n");
  // Check if socket exists
  if (isActive && (socketDesc >= 0)) {
     // Drop the existing multicast group
     struct ip_mreq mreq;
     mreq.imr_multiaddr.s_addr = streamAddr;
     mreq.imr_interface.s_addr = htonl(INADDR_ANY);
     ERROR_IF_RET(setsockopt(socketDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0, "setsockopt(IP_DROP_MEMBERSHIP)", return false);
     // Update multicasting flag
     isActive = false;
     }
  return true;
}


int cIptvUdpSocket::Read(unsigned char* BufferAddr, unsigned int BufferLen)
{
  //debug("cIptvUdpSocket::Read()\n");
  // Error out if socket not initialized
  if (socketDesc <= 0) {
     error("Invalid socket in %s\n", __FUNCTION__);
     return -1;
     }
  int len = 0;
  // Read data from socket in a loop
  do {
    socklen_t addrlen = sizeof(sockAddr);
    struct msghdr msgh;
    struct cmsghdr *cmsg;
    struct iovec iov;
    char cbuf[256];
    len = 0;
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

    if (isActive && socketDesc && BufferAddr && (BufferLen > 0))
       len = (int)recvmsg(socketDesc, &msgh, MSG_DONTWAIT);
    else
       break;
    if (len > 0) {
       // Process auxiliary received data and validate source address
       for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
           if ((cmsg->cmsg_level == SOL_IP) && (cmsg->cmsg_type == IP_PKTINFO)) {
              struct in_pktinfo *i = (struct in_pktinfo *)CMSG_DATA(cmsg);
              if ((i->ipi_addr.s_addr == streamAddr) || (INADDR_ANY == streamAddr)) {
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
              }
           }
       }
    } while (len > 0);
  ERROR_IF_RET(len < 0 && errno != EAGAIN, "recvmsg()", return -1);

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

bool cIptvTcpSocket::OpenSocket(const int Port, const char *StreamAddr)
{
  debug("cIptvTcpSocket::OpenSocket()\n");

  // Socket must be opened before setting the host address
  bool retval = cIptvSocket::OpenSocket(Port, false);

  // First try only the IP address
  sockAddr.sin_addr.s_addr = inet_addr(StreamAddr);

  if (sockAddr.sin_addr.s_addr == INADDR_NONE) {
     debug("Cannot convert %s directly to internet address\n", StreamAddr);

     // It may be a host name, get the name
     struct hostent *host;
     host = gethostbyname(StreamAddr);
     if (!host) {
        char tmp[64];
        error("gethostbyname() failed: %s is not valid address: %s", StreamAddr, strerror_r(h_errno, tmp, sizeof(tmp)));
        return false;
        }

     sockAddr.sin_addr.s_addr = inet_addr(*host->h_addr_list);
     }

  return retval;
}

void cIptvTcpSocket::CloseSocket(void)
{
  debug("cIptvTcpSocket::CloseSocket()\n");
  cIptvSocket::CloseSocket();
}

bool cIptvTcpSocket::ConnectSocket(void)
{
  debug("cIptvTcpSocket::ConnectSocket()\n");
  if (!isActive && (socketDesc >= 0)) {
     int retval = connect(socketDesc, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
     // Non-blocking sockets always report in-progress error when connected
     ERROR_IF_RET(retval < 0 && errno != EINPROGRESS, "connect()", return false);
     // Select with 800ms timeout on the socket completion, check if it is writable
     retval = select_single_desc(socketDesc, 800000, true);
     if (retval < 0)
        return retval;
     // Select has returned. Get socket errors if there are any
     retval = 0;
     socklen_t len = sizeof(retval);
     getsockopt(socketDesc, SOL_SOCKET, SO_ERROR, &retval, &len);
     // If not any errors, then socket must be ready and connected
     if (retval != 0) {
        char tmp[64];
        error("Connect() failed: %s", strerror_r(retval, tmp, sizeof(tmp)));
        return false;
        }
     }

  return true;
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
  return len;
}

bool cIptvTcpSocket::ReadChar(char* BufferAddr, unsigned int TimeoutMs)
{
  //debug("cIptvTcpSocket::ReadChar()\n");
  // Error out if socket not initialized
  if (socketDesc <= 0) {
     error("Invalid socket in %s\n", __FUNCTION__);
     return false;
     }
  socklen_t addrlen = sizeof(sockAddr);
  // Wait 500ms for data
  int retval = select_single_desc(socketDesc, 1000 * TimeoutMs, false);
  // Check if error
  if (retval < 0)
     return false;
  // Check if data available
  else if (retval) {
     retval = (int)recvfrom(socketDesc, BufferAddr, 1, MSG_DONTWAIT,
                            (struct sockaddr *)&sockAddr, &addrlen);
     if (retval <= 0)
        return false;
     }

  return true;
}

bool cIptvTcpSocket::Write(const char* BufferAddr, unsigned int BufferLen)
{
  //debug("cIptvTcpSocket::Write()\n");
  // Error out if socket not initialized
  if (socketDesc <= 0) {
     error("Invalid socket in %s\n", __FUNCTION__);
     return false;
     }
  ERROR_IF_RET(send(socketDesc, BufferAddr, BufferLen, 0) < 0, "send()", return false);

  return true;
}
