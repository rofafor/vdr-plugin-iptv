/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.c,v 1.10 2007/09/13 16:58:22 rahrenbe Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>
#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

#include "common.h"
#include "streamer.h"

cIptvStreamer::cIptvStreamer(cRingBufferLinear* Buffer, cMutex* Mutex)
: cThread("IPTV streamer"),
  streamPort(1234),
  socketDesc(-1),
  pRingBuffer(Buffer),
  bufferSize(TS_SIZE * 7),
  mutex(Mutex),
  mcastActive(false)
{
  debug("cIptvStreamer::cIptvStreamer()\n");
  streamAddr = strdup("");
  // Create the socket
  OpenSocket(streamPort);
  // Allocate receive buffer
  pReceiveBuffer = MALLOC(unsigned char, bufferSize);
  if (!pReceiveBuffer)
     error("ERROR: MALLOC(pReceiveBuffer) failed");
}

cIptvStreamer::~cIptvStreamer()
{
  debug("cIptvStreamer::~cIptvStreamer()\n");
  // Drop the multicast group
  DropMulticast();
  // Close the stream and socket
  CloseStream();
  CloseSocket();
  // Free allocated memory
  free(streamAddr);
  free(pReceiveBuffer);
}

void cIptvStreamer::Action(void)
{
  debug("cIptvStreamer::Action(): Entering\n");
  // Create files necessary for selecting I/O from socket
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(socketDesc, &rfds);
  // Do the thread loop
  while (Running()) {
    if (pRingBuffer && mutex && pReceiveBuffer && (socketDesc >= 0)) {
       struct timeval tv;
       socklen_t addrlen = sizeof(sockAddr);
       // Wait for data
       tv.tv_sec = 0;
       tv.tv_usec = 500000;
       int retval = select(socketDesc + 1, &rfds, NULL, NULL, &tv);
       if (retval < 0) {
          char tmp[64];
          error("ERROR: select(): %s", strerror_r(errno, tmp, sizeof(tmp)));
       } else if (retval) {
          // Read data from socket
          int length = recvfrom(socketDesc, pReceiveBuffer, bufferSize, MSG_DONTWAIT,
                                (struct sockaddr *)&sockAddr, &addrlen);
          mutex->Lock();
          int p = pRingBuffer->Put(pReceiveBuffer, length);
          if (p != length && Running())
             pRingBuffer->ReportOverflow(length - p);
          mutex->Unlock();
       } else
          debug("cIptvStreamer::Action(): Timeout\n");
       }
    else
       cCondWait::SleepMs(100); // avoid busy loop
    }
  debug("cIptvStreamer::Action(): Exiting\n");
}

bool cIptvStreamer::OpenSocket(const int port)
{
  debug("cIptvStreamer::OpenSocket()\n");
  // If socket is there already and it is bound to a different port, it must
  // be closed first
  if (port != streamPort) {
     debug("cIptvStreamer::OpenSocket(): Socket tear-down\n");
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
     sockAddr.sin_port = htons(port);
     sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     int err = bind(socketDesc, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
     if (err < 0) {
        char tmp[64];
        error("ERROR: bind(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }
     // Update stream port
     streamPort = port;
     }
  return true;
}

void cIptvStreamer::CloseSocket(void)
{
  debug("cIptvStreamer::CloseSocket()\n");
  // Check if socket exists
  if (socketDesc >= 0) {
     close(socketDesc);
     socketDesc = -1;
     }
}

bool cIptvStreamer::JoinMulticast(void)
{
  debug("cIptvStreamer::JoinMulticast()\n");
  // Check that stream address is valid
  if (!mcastActive && !isempty(streamAddr)) {
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
     mcastActive = true;
     }
  return true;
}

bool cIptvStreamer::DropMulticast(void)
{
  debug("cIptvStreamer::DropMulticast()\n");
  // Check that stream address is valid
  if (mcastActive && !isempty(streamAddr)) {
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
      mcastActive = false;
     }
  return true;
}

bool cIptvStreamer::OpenStream(void)
{
  debug("cIptvStreamer::OpenStream(): streamAddr = %s\n", streamAddr);
  // Join a new multicast group
  JoinMulticast();
  // Start thread
  Start();
  return true;
}

bool cIptvStreamer::CloseStream(void)
{
  debug("cIptvStreamer::CloseStream(): streamAddr = %s\n", streamAddr);
  // Stop thread
  if (Running())
     Cancel(3);
  // Drop the multicast group
  DropMulticast();
  return true;
}

bool cIptvStreamer::SetStream(const char* address, const int port, const char* protocol)
{
  debug("cIptvStreamer::SetStream(): %s://%s:%d\n", protocol, address, port);
  if (!isempty(address)) {
    // Drop the multicast group
    DropMulticast();
    // Update stream address and port
    streamAddr = strcpyrealloc(streamAddr, address);
    streamPort = port;
    // Join a new multicast group
    JoinMulticast();
    }
  return true;
}

