/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.c,v 1.2 2007/09/12 18:05:58 ajhseppa Exp $
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

cIptvStreamer::cIptvStreamer(cRingBufferLinear* BufferPtr, cMutex* Mutex)
: cThread("IPTV streamer"),
  dataPort(1234),
  dataProtocol(PROTOCOL_UDP),
  pRingBuffer(BufferPtr),
  bufferSize(TS_SIZE * 7),
  mutex(Mutex),
  socketActive(false),
  mcastActive(false)
{
  int yes = 1;

  debug("cIptvStreamer::cIptvStreamer()\n");
  memset(&stream, '\0', strlen(stream));

  // Create socket
  socketDesc = socket(PF_INET, SOCK_DGRAM, 0);
  if (socketDesc < 0) {
     char tmp[64];
     error("ERROR: socket(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     }

  // Allow multiple sockets to use the same PORT number
  if (setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
     char tmp[64];
     error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     }

  pReceiveBuffer = MALLOC(unsigned char, bufferSize);
  if (!pReceiveBuffer)
     error("ERROR: MALLOC(pReceiveBuffer) failed");
}

cIptvStreamer::~cIptvStreamer()
{
  debug("cIptvStreamer::~cIptvStreamer()\n");

  if (Running())
     Cancel(3);

  if (pReceiveBuffer)
     free(pReceiveBuffer);

  close(socketDesc);
  socketActive = false;
}

void cIptvStreamer::Action()
{
  debug("cIptvStreamer::Action(): Entering\n");
  while (Running()) {
    socklen_t addrlen = sizeof(sa);
    int length = recvfrom(socketDesc, pReceiveBuffer, bufferSize, 0, (struct sockaddr *)&sa, &addrlen);
    mutex->Lock();
    int p = pRingBuffer->Put(pReceiveBuffer, bufferSize);
    if (p != length && Running()) {
       pRingBuffer->ReportOverflow(length - p);
       debug("Reporting overflow\n");
       }
    mutex->Unlock();
    }
  debug("cIptvStreamer::Action(): Exiting\n");
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
     debug("cIptvStreamer::Activate(): Already active!\n");
     return true;
     }

  // Start thread
  if (!Running())
     Start();

  // Join a new multicast group
  mreq.imr_multiaddr.s_addr = inet_addr(stream);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);

  int err = setsockopt(socketDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
  if (err < 0) {
     char tmp[64];
     error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     return false;
     }

  mcastActive = true;
  return true;
}

bool cIptvStreamer::Deactivate()
{
  debug("cIptvStreamer::Deactivate()\n");
  if (stream && mcastActive) {
     debug("cIptvStreamer::Deactivate(): stream = %s\n", stream);

     struct ip_mreq mreq;
     mreq.imr_multiaddr.s_addr = inet_addr(stream);
     mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
     int err = setsockopt(socketDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
     if (err < 0) {
        char tmp[64];
        error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return false;
        }

     // Not active any more
     mcastActive = false;
     }

  // Stop thread
  if (Running())
     Cancel(3);

  return true;
}

bool cIptvStreamer::SetStream(const char* address, const int port, const int protocol)
{
  debug("cIptvStreamer::SetChannel(): channel = %s:%d\n", address, port);

  if (port != dataPort) {
     error("ERROR: Support for full re-initialization is not implemented!\n");
     return false;
     }

  // Bind to the socket if it is not active already
  if (!socketActive) {
     debug("cIptvStreamer::SetChannel(): Binding socket to %s:%d\n", address, port);
     dataPort = port;

     memset(&sa, '\0', sizeof(sa));
     sa.sin_family = AF_INET;
     sa.sin_port = htons(dataPort);
     sa.sin_addr.s_addr = htonl(INADDR_ANY);

     int err = bind(socketDesc, (struct sockaddr *)&sa, sizeof(sa));
     if (err < 0) {
        char tmp[64];
        error("ERROR: bind(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return false;
        }

     socketActive = true;
     }

  // De-activate the reception if it is running currently. Otherwise the
  // reception stream is overwritten and cannot be un-set after this
  Deactivate();

  // Check if the address fits into the buffer
  if (strlen(address) > sizeof(stream)) {
     error("ERROR: Address too big\n");
     return false;
     }
  strn0cpy(stream, address, sizeof(stream));

  return true;
}

