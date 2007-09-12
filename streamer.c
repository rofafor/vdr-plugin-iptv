/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.c,v 1.5 2007/09/12 18:34:43 rahrenbe Exp $
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
        }

     int yes = 1;     

     // Allow multiple sockets to use the same PORT number
     if (setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &yes,
		    sizeof(yes)) < 0) {
        char tmp[64];
        error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        }

     memset(&sa, '\0', sizeof(sa));
     sa.sin_family = AF_INET;
     sa.sin_port = htons(port);
     sa.sin_addr.s_addr = htonl(INADDR_ANY);

     int err = bind(socketDesc, (struct sockaddr *)&sa, sizeof(sa));
     if (err < 0) {
        char tmp[64];
        error("ERROR: bind(): %s", strerror_r(errno, tmp, sizeof(tmp)));
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
     debug("cIptvStreamer::Activate(): Already active!\n");
     return true;
     }

  // Start thread
  if (!Running())
     Start();

  // Ensure that socket is valid
  CheckAndCreateSocket(dataPort);

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
  debug("cIptvStreamer::SetChannel(): channel = %s:%d (%d)\n", address, port, protocol);

  // De-activate the reception if it is running currently. Otherwise the
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

