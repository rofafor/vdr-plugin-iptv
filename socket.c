/*
 * socket.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>

#include "common.h"
#include "config.h"
#include "log.h"
#include "socket.h"

cIptvSocket::cIptvSocket()
: socketPortM(0),
  socketDescM(-1),
  lastErrorReportM(0),
  packetErrorsM(0),
  sequenceNumberM(-1),
  isActiveM(false)
{
  debug1("%s", __PRETTY_FUNCTION__);
  memset(&sockAddrM, 0, sizeof(sockAddrM));
}

cIptvSocket::~cIptvSocket()
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Close the socket
  CloseSocket();
}

bool cIptvSocket::OpenSocket(const int portP, const bool isUdpP)
{
  debug1("%s (%d, %d)", __PRETTY_FUNCTION__, portP, isUdpP);
  // If socket is there already and it is bound to a different port, it must
  // be closed first
  if (portP != socketPortM) {
     debug1("%s (%d, %d) Socket tear-down", __PRETTY_FUNCTION__, portP, isUdpP);
     CloseSocket();
     }
  // Bind to the socket if it is not active already
  if (socketDescM < 0) {
     int yes = 1;
     // Create socket
     if (isUdpP)
        socketDescM = socket(PF_INET, SOCK_DGRAM, 0);
     else
        socketDescM = socket(PF_INET, SOCK_STREAM, 0);
     ERROR_IF_RET(socketDescM < 0, "socket()", return false);
     // Make it use non-blocking I/O to avoid stuck read calls
     ERROR_IF_FUNC(fcntl(socketDescM, F_SETFL, O_NONBLOCK), "fcntl(O_NONBLOCK)",
                   CloseSocket(), return false);
     // Allow multiple sockets to use the same PORT number
     ERROR_IF_FUNC(setsockopt(socketDescM, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0,
                   "setsockopt(SO_REUSEADDR)", CloseSocket(), return false);
#ifndef __FreeBSD__
     // Allow packet information to be fetched
     ERROR_IF_FUNC(setsockopt(socketDescM, SOL_IP, IP_PKTINFO, &yes, sizeof(yes)) < 0,
                   "setsockopt(IP_PKTINFO)", CloseSocket(), return false);
#endif // __FreeBSD__
     // Bind socket
     memset(&sockAddrM, 0, sizeof(sockAddrM));
     sockAddrM.sin_family = AF_INET;
     sockAddrM.sin_port = htons((uint16_t)(portP & 0xFFFF));
     sockAddrM.sin_addr.s_addr = htonl(INADDR_ANY);
     if (isUdpP)
        ERROR_IF_FUNC(bind(socketDescM, (struct sockaddr *)&sockAddrM, sizeof(sockAddrM)) < 0,
                      "bind()", CloseSocket(), return false);
     // Update socket port
     socketPortM = portP;
     }
  return true;
}

void cIptvSocket::CloseSocket(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Check if socket exists
  if (socketDescM >= 0) {
     close(socketDescM);
     socketDescM = -1;
     socketPortM = 0;
     memset(&sockAddrM, 0, sizeof(sockAddrM));
     }
  if (packetErrorsM) {
     info("Detected %d RTP packet errors", packetErrorsM);
     packetErrorsM = 0;
     lastErrorReportM = time(NULL);
     }
}

bool cIptvSocket::CheckAddress(const char *addrP, in_addr_t *inAddrP)
{
  if (inAddrP) {
     // First try only the IP address
     *inAddrP = inet_addr(addrP);
     if (*inAddrP == htonl(INADDR_NONE)) {
        debug1("%s (%s, ) Cannot convert to address", __PRETTY_FUNCTION__, addrP);
        // It may be a host name, get the name
        struct hostent *host = gethostbyname(addrP);
        if (!host) {
           char tmp[64];
           error("gethostbyname() failed: %s is not valid address: %s", addrP,
                 strerror_r(h_errno, tmp, sizeof(tmp)));
           return false;
           }
        *inAddrP = inet_addr(*host->h_addr_list);
        }
     return true;
     }
  return false;
}

// UDP socket class
cIptvUdpSocket::cIptvUdpSocket()
: streamAddrM(htonl(INADDR_ANY)),
  sourceAddrM(htonl(INADDR_ANY)),
  useIGMPv3M(false)
{
  debug1("%s", __PRETTY_FUNCTION__);
}

cIptvUdpSocket::~cIptvUdpSocket()
{
  debug1("%s", __PRETTY_FUNCTION__);
}

bool cIptvUdpSocket::OpenSocket(const int portP)
{
  debug1("%s (%d)", __PRETTY_FUNCTION__, portP);
  streamAddrM = htonl(INADDR_ANY);
  sourceAddrM = htonl(INADDR_ANY);
  useIGMPv3M = false;
  return cIptvSocket::OpenSocket(portP, true);
}

bool cIptvUdpSocket::OpenSocket(const int portP, const char *streamAddrP, const char *sourceAddrP, bool useIGMPv3P)
{
  debug1("%s (%d, %s, %s, %d)", __PRETTY_FUNCTION__, portP, streamAddrP, sourceAddrP, useIGMPv3P);
  CheckAddress(streamAddrP, &streamAddrM);
  CheckAddress(sourceAddrP, &sourceAddrM);
  useIGMPv3M = useIGMPv3P;
  return cIptvSocket::OpenSocket(portP, true);
}

void cIptvUdpSocket::CloseSocket(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  streamAddrM = htonl(INADDR_ANY);
  sourceAddrM = htonl(INADDR_ANY);
  useIGMPv3M = false;
  cIptvSocket::CloseSocket();
}

bool cIptvUdpSocket::JoinMulticast(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Check if socket exists
  if (!isActiveM && (socketDescM >= 0)) {
     // Join a new multicast group
     if (useIGMPv3M) {
        // Source-specific multicast (SSM) is used
        struct group_source_req gsr;
        struct sockaddr_in *grp;
        struct sockaddr_in *src;
        gsr.gsr_interface = 0; // if_nametoindex("any") ?
        grp = (struct sockaddr_in*)&gsr.gsr_group;
        grp->sin_family = AF_INET;
        grp->sin_addr.s_addr = streamAddrM;
        grp->sin_port = 0;
        src = (struct sockaddr_in*)&gsr.gsr_source;
        src->sin_family = AF_INET;
        src->sin_addr.s_addr = sourceAddrM;
        src->sin_port = 0;
        ERROR_IF_RET(setsockopt(socketDescM, SOL_IP, MCAST_JOIN_SOURCE_GROUP, &gsr, sizeof(gsr)) < 0, "setsockopt(MCAST_JOIN_SOURCE_GROUP)", return false);
        }
     else {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = streamAddrM;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        ERROR_IF_RET(setsockopt(socketDescM, SOL_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0, "setsockopt(IP_ADD_MEMBERSHIP)", return false);
        }
     // Update multicasting flag
     isActiveM = true;
     }
  return true;
}

bool cIptvUdpSocket::DropMulticast(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Check if socket exists
  if (isActiveM && (socketDescM >= 0)) {
     // Drop the existing multicast group
     if (useIGMPv3M) {
        // Source-specific multicast (SSM) is used
        struct group_source_req gsr;
        struct sockaddr_in *grp;
        struct sockaddr_in *src;
        gsr.gsr_interface = 0; // if_nametoindex("any") ?
        grp = (struct sockaddr_in*)&gsr.gsr_group;
        grp->sin_family = AF_INET;
        grp->sin_addr.s_addr = streamAddrM;
        grp->sin_port = 0;
        src = (struct sockaddr_in*)&gsr.gsr_source;
        src->sin_family = AF_INET;
        src->sin_addr.s_addr = sourceAddrM;
        src->sin_port = 0;
        ERROR_IF_RET(setsockopt(socketDescM, SOL_IP, MCAST_LEAVE_SOURCE_GROUP, &gsr, sizeof(gsr)) < 0, "setsockopt(MCAST_LEAVE_SOURCE_GROUP)", return false);
        }
     else {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = streamAddrM;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        ERROR_IF_RET(setsockopt(socketDescM, SOL_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0, "setsockopt(IP_DROP_MEMBERSHIP)", return false);
        }
     // Update multicasting flag
     isActiveM = false;
     }
  return true;
}


int cIptvUdpSocket::Read(unsigned char *bufferAddrP, unsigned int bufferLenP)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Error out if socket not initialized
  if (socketDescM <= 0) {
     error("%s Invalid socket", __PRETTY_FUNCTION__);
     return -1;
     }
  int len = 0;
  // Read data from socket in a loop
  do {
    socklen_t addrlen = sizeof(sockAddrM);
    struct msghdr msgh;
    struct iovec iov;
    char cbuf[256];
    len = 0;
    // Initialize iov and msgh structures
    memset(&msgh, 0, sizeof(struct msghdr));
    iov.iov_base = bufferAddrP;
    iov.iov_len = bufferLenP;
    msgh.msg_control = cbuf;
    msgh.msg_controllen = sizeof(cbuf);
    msgh.msg_name = &sockAddrM;
    msgh.msg_namelen = addrlen;
    msgh.msg_iov  = &iov;
    msgh.msg_iovlen = 1;
    msgh.msg_flags = 0;

    if (isActiveM && socketDescM && bufferAddrP && (bufferLenP > 0))
       len = (int)recvmsg(socketDescM, &msgh, MSG_DONTWAIT);
    else
       break;
    if (len > 0) {
#ifndef __FreeBSD__
       // Process auxiliary received data and validate source address
       for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
           if ((cmsg->cmsg_level == SOL_IP) && (cmsg->cmsg_type == IP_PKTINFO)) {
              struct in_pktinfo *i = (struct in_pktinfo *)CMSG_DATA(cmsg);
              if ((i->ipi_addr.s_addr == streamAddrM) || (htonl(INADDR_ANY) == streamAddrM)) {
#endif // __FreeBSD__
                 if (bufferAddrP[0] == TS_SYNC_BYTE)
                    return len;
                 else if (len > 3) {
                    // http://tools.ietf.org/html/rfc3550
                    // http://tools.ietf.org/html/rfc2250
                    // Version
                    unsigned int v = (bufferAddrP[0] >> 6) & 0x03;
                    // Extension bit
                    unsigned int x = (bufferAddrP[0] >> 4) & 0x01;
                    // CSCR count
                    unsigned int cc = bufferAddrP[0] & 0x0F;
                    // Payload type: MPEG2 TS = 33
                    //unsigned int pt = bufferAddrP[1] & 0x7F;
                    // Sequence number
                    int seq = ((bufferAddrP[2] & 0xFF) << 8) | (bufferAddrP[3] & 0xFF);
                    if ((((sequenceNumberM + 1) % 0xFFFF) == 0) && (seq == 0xFFFF))
                       sequenceNumberM = -1;
                    else if ((sequenceNumberM >= 0) && (((sequenceNumberM + 1) % 0xFFFF) != seq)) {
                       packetErrorsM++;
                       if (time(NULL) - lastErrorReportM > eReportIntervalS) {
                          info("Detected %d RTP packet errors", packetErrorsM);
                          packetErrorsM = 0;
                          lastErrorReportM = time(NULL);
                          }
                       sequenceNumberM = seq;
                       }
                    else
                       sequenceNumberM = seq;
                    // Header lenght
                    unsigned int headerlen = (3 + cc) * (unsigned int)sizeof(uint32_t);
                    // Check if extension
                    if (x) {
                       // Extension header length
                       unsigned int ehl = (((bufferAddrP[headerlen + 2] & 0xFF) << 8) |
                                           (bufferAddrP[headerlen + 3] & 0xFF));
                       // Update header length
                       headerlen += (ehl + 1) * (unsigned int)sizeof(uint32_t);
                       }
                    // Check that rtp is version 2 and payload contains multiple of TS packet data
                    if ((v == 2) && (((len - headerlen) % TS_SIZE) == 0) &&
                        (bufferAddrP[headerlen] == TS_SYNC_BYTE)) {
                       // Set argument point to payload in read buffer
                       memmove(bufferAddrP, &bufferAddrP[headerlen], (len - headerlen));
                       return (len - headerlen);
                       }
                    }
#ifndef __FreeBSD__
                 }
              }
           }
#endif // __FreeBSD__
       }
    } while (len > 0);
  ERROR_IF_RET(len < 0 && errno != EAGAIN, "recvmsg()", return -1);

  return 0;
}

// TCP socket class
cIptvTcpSocket::cIptvTcpSocket()
{
  debug1("%s", __PRETTY_FUNCTION__);
}

cIptvTcpSocket::~cIptvTcpSocket()
{
  debug1("%s", __PRETTY_FUNCTION__);
}

bool cIptvTcpSocket::OpenSocket(const int portP, const char *streamAddrP)
{
  debug1("%s (%d, %s)", __PRETTY_FUNCTION__, portP, streamAddrP);
  // Socket must be opened before setting the host address
  return (cIptvSocket::OpenSocket(portP, false) && CheckAddress(streamAddrP, &sockAddrM.sin_addr.s_addr));
}

void cIptvTcpSocket::CloseSocket(void)
{
  debug1("%s()", __PRETTY_FUNCTION__);
  cIptvSocket::CloseSocket();
}

bool cIptvTcpSocket::ConnectSocket(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  if (!isActiveM && (socketDescM >= 0)) {
     int retval = connect(socketDescM, (struct sockaddr *)&sockAddrM, sizeof(sockAddrM));
     // Non-blocking sockets always report in-progress error when connected
     ERROR_IF_RET(retval < 0 && errno != EINPROGRESS, "connect()", return false);
     // Select with 800ms timeout on the socket completion, check if it is writable
     retval = select_single_desc(socketDescM, 800000, true);
     if (retval < 0)
        return retval;
     // Select has returned. Get socket errors if there are any
     retval = 0;
     socklen_t len = sizeof(retval);
     getsockopt(socketDescM, SOL_SOCKET, SO_ERROR, &retval, &len);
     // If not any errors, then socket must be ready and connected
     if (retval != 0) {
        char tmp[64];
        error("Connect() failed: %s", strerror_r(retval, tmp, sizeof(tmp)));
        return false;
        }
     }

  return true;
}

int cIptvTcpSocket::Read(unsigned char *bufferAddrP, unsigned int bufferLenP)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Error out if socket not initialized
  if (socketDescM <= 0) {
     error("%s Invalid socket", __PRETTY_FUNCTION__);
     return -1;
     }
  int len = 0;
  socklen_t addrlen = sizeof(sockAddrM);
  // Read data from socket
  if (isActiveM && socketDescM && bufferAddrP && (bufferLenP > 0))
     len = (int)recvfrom(socketDescM, bufferAddrP, bufferLenP, MSG_DONTWAIT,
                         (struct sockaddr *)&sockAddrM, &addrlen);
  return len;
}

bool cIptvTcpSocket::ReadChar(char *bufferAddrP, unsigned int timeoutMsP)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Error out if socket not initialized
  if (socketDescM <= 0) {
     error("%s Invalid socket", __PRETTY_FUNCTION__);
     return false;
     }
  socklen_t addrlen = sizeof(sockAddrM);
  // Wait 500ms for data
  int retval = select_single_desc(socketDescM, 1000 * timeoutMsP, false);
  // Check if error
  if (retval < 0)
     return false;
  // Check if data available
  else if (retval) {
     retval = (int)recvfrom(socketDescM, bufferAddrP, 1, MSG_DONTWAIT,
                            (struct sockaddr *)&sockAddrM, &addrlen);
     if (retval <= 0)
        return false;
     }

  return true;
}

bool cIptvTcpSocket::Write(const char *bufferAddrP, unsigned int bufferLenP)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Error out if socket not initialized
  if (socketDescM <= 0) {
     error("%s Invalid socket", __PRETTY_FUNCTION__);
     return false;
     }
  ERROR_IF_RET(send(socketDescM, bufferAddrP, bufferLenP, 0) < 0, "send()", return false);

  return true;
}
