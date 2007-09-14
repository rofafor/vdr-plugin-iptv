/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.c,v 1.11 2007/09/14 15:44:25 rahrenbe Exp $
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

cIptvStreamer::cIptvStreamer(cRingBufferLinear* RingBuffer, cMutex* Mutex)
: cThread("IPTV streamer"),
  ringBuffer(RingBuffer),
  mutex(Mutex),
  readBufferLen(TS_SIZE * 7),
  protocol(NULL)
{
  debug("cIptvStreamer::cIptvStreamer()\n");
  // Allocate receive buffer
  readBuffer = MALLOC(unsigned char, readBufferLen);
  if (!readBuffer)
     error("ERROR: MALLOC(readBuffer) failed");
}

cIptvStreamer::~cIptvStreamer()
{
  debug("cIptvStreamer::~cIptvStreamer()\n");
  // Close the protocol
  Close();
  // Free allocated memory
  free(readBuffer);
}

void cIptvStreamer::Action(void)
{
  debug("cIptvStreamer::Action(): Entering\n");
  // Do the thread loop
  while (Running()) {
    if (ringBuffer && mutex && readBuffer && protocol) {
       int length = protocol->Read(readBuffer, readBufferLen);
       if (length >= 0) {
          mutex->Lock();
          int p = ringBuffer->Put(readBuffer, length);
          if (p != length && Running())
             ringBuffer->ReportOverflow(length - p);
          mutex->Unlock();
          }
       else
          cCondWait::SleepMs(3); // reduce cpu load
       }
    else
       cCondWait::SleepMs(100); // avoid busy loop
    }
  debug("cIptvStreamer::Action(): Exiting\n");
}

bool cIptvStreamer::Open(void)
{
  debug("cIptvStreamer::Open()\n");
  // Open the protocol
  if (protocol)
     protocol->Open();
  // Start thread
  Start();
  return true;
}

bool cIptvStreamer::Close(void)
{
  debug("cIptvStreamer::Close()\n");
  // Stop thread
  if (Running())
     Cancel(3);
  // Close the protocol
  if (protocol)
     protocol->Close();
  return true;
}

bool cIptvStreamer::Set(const char* Address, const int Port, cIptvProtocolIf* Protocol)
{
  debug("cIptvStreamer::Set(): %s:%d\n", Address, Port);
  if (!isempty(Address)) {
    // Update protocol; Close the existing one if changed
    if (protocol != Protocol) {
       if (protocol)
          protocol->Close();
       protocol = Protocol;
       if (protocol)
          protocol->Open();
       }
    // Set protocol address and port
    if (protocol)
        protocol->Set(Address, Port);
    }
  return true;
}

