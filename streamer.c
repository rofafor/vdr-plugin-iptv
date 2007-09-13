/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.c,v 1.9 2007/09/13 14:10:37 ajhseppa Exp $
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
  dataPort(1234),
  pRingBuffer(Buffer),
  bufferSize(TS_SIZE * 7),
  mutex(Mutex),
  socketActive(false),
  mcastActive(false)
{

  debug("cIptvStreamer::cIptvStreamer()\n");
  memset(&stream, '\0', strlen(stream));

  // Create the socket
  CheckAndCreateSocket(dataPort);

  pReceiveBuffer = MALLOC(unsigned char, bufferSize);
  if (!pReceiveBuffer)
     error("ERROR: MALLOC(pReceiveBuffer) failed");
}

cIptvStreamer::~cIptvStreamer()
{
  debug("cIptvStreamer::~cIptvStreamer()\n");

  Deactivate();

  if (pReceiveBuffer)
     free(pReceiveBuffer);

  // Close the socket
  CloseSocket();
}

void cIptvStreamer::Action()
{
  debug("cIptvStreamer::Action(): Entering\n");

  // Create files necessary for selecting I/O from socket.
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(socketDesc, &rfds);

  while (Running()) {
    socklen_t addrlen = sizeof(sa);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    // Wait for data
    int retval = select(socketDesc + 1, &rfds, NULL, NULL, &tv);

    if (retval < 0) {
      char tmp[64];
      error("ERROR: select(): %s", strerror_r(errno, tmp, sizeof(tmp)));
    } else if(retval) {

      // Read data from socket
      int length = recvfrom(socketDesc, pReceiveBuffer, bufferSize,
			    MSG_DONTWAIT, (struct sockaddr *)&sa, &addrlen);
      mutex->Lock();
      int p = pRingBuffer->Put(pReceiveBuffer, length);
      if (p != length && Running()) {
	pRingBuffer->ReportOverflow(length - p);
        }
      mutex->Unlock();

    } else {
      debug("Timeout waiting for data\n");
    }
  }
  debug("cIptvStreamer::Action(): Exiting\n");
}

bool cIptvStreamer::CheckAndCreateSocket(const int port)
{
  // If socket is there already and it is bound to a different port, it must
  // be closed first
  if (socketActive && port != dataPort) {
     debug("Full tear-down of active socket\n");
     CloseSocket();
     }

  // Bind to the socket if it is not active already
  if (!socketActive) {

     // Create socket
     socketDesc = socket(PF_INET, SOCK_DGRAM, 0);
     if (socketDesc < 0) {
        char tmp[64];
        error("ERROR: socket(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return false;
        }

     // Make it use non-blocking I/O to avoid stuck read -calls.
     if (fcntl(socketDesc, F_SETFL, O_NONBLOCK)) {
        char tmp[64];
        error("ERROR: fcntl(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        close(socketDesc);
        return false;
        }

     int yes = 1;     

     // Allow multiple sockets to use the same PORT number
     if (setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &yes,
		    sizeof(yes)) < 0) {
        char tmp[64];
        error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        close(socketDesc);
        return false;
        }

     memset(&sa, '\0', sizeof(sa));
     sa.sin_family = AF_INET;
     sa.sin_port = htons(port);
     sa.sin_addr.s_addr = htonl(INADDR_ANY);

     int err = bind(socketDesc, (struct sockaddr *)&sa, sizeof(sa));
     if (err < 0) {
        char tmp[64];
        error("ERROR: bind(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        close(socketDesc);
        return false;
        }

     dataPort = port;
     socketActive = true;
     }

  return true;

}

void cIptvStreamer::CloseSocket()
{
  if (socketActive) {
    close(socketDesc);
    socketActive = false;
    mcastActive = false;
  }
}

bool cIptvStreamer::Activate()
{
  struct ip_mreq mreq;

  debug("cIptvStreamer::Activate(): stream = %s\n", stream);

  if (!stream) {
     error("No stream set yet, not activating\n");
     return false;
     } 

  if (mcastActive) {
     debug("cIptvStreamer::Activate(): Already active\n");
     return true;
     }

  // Ensure that socket is valid
  CheckAndCreateSocket(dataPort);

  // Join a new multicast group
  mreq.imr_multiaddr.s_addr = inet_addr(stream);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  int err = setsockopt(socketDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                       sizeof(mreq));
  if (err < 0) {
     char tmp[64];
     error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     return false;
     }

  // Start thread
  if (!Running())
     Start();

  mcastActive = true;
  return true;
}

bool cIptvStreamer::Deactivate()
{
  debug("cIptvStreamer::Deactivate(): stream = %s\n", stream);

  // Stop thread
  if (Running())
     Cancel(3);

  if (stream && mcastActive) {
     struct ip_mreq mreq;
     debug("cIptvStreamer::Deactivate(): Deactivating\n");
     mreq.imr_multiaddr.s_addr = inet_addr(stream);
     mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
     int err = setsockopt(socketDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,
                          sizeof(mreq));
     if (err < 0) {
        char tmp[64];
        error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return false;
        }

     // Not active any more
     mcastActive = false;
     }

  return true;
}

bool cIptvStreamer::SetStream(const char* address, const int port, const char* protocol)
{
  debug("cIptvStreamer::SetStream(): %s://%s:%d\n", protocol, address, port);

  // Deactivate the reception if it is running currently. Otherwise the
  // reception stream is overwritten and cannot be un-set after this
  Deactivate();

  // Ensure that the socket is valid
  CheckAndCreateSocket(port);

  // Check if the address fits into the buffer
  if (strlen(address) > sizeof(stream)) {
     error("ERROR: Address too big\n");
     return false;
     }
  strn0cpy(stream, address, sizeof(stream));

  return true;
}

