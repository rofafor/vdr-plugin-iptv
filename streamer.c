/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.c,v 1.20 2007/10/09 17:58:17 ajhseppa Exp $
 */

#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

#include "common.h"
#include "streamer.h"

cIptvStreamer::cIptvStreamer(cRingBufferLinear* RingBuffer, cMutex* Mutex)
: cThread("IPTV streamer"),
  ringBuffer(RingBuffer),
  mutex(Mutex),
  protocol(NULL)
{
  debug("cIptvStreamer::cIptvStreamer()\n");
}

cIptvStreamer::~cIptvStreamer()
{
  debug("cIptvStreamer::~cIptvStreamer()\n");
  // Close the protocol
  Close();
}

void cIptvStreamer::Action(void)
{
  debug("cIptvStreamer::Action(): Entering\n");
  // Do the thread loop
  while (Running()) {
    if (ringBuffer && mutex && protocol && ringBuffer->Free()) {
       unsigned char *buffer = NULL;
       int length = protocol->Read(&buffer);
       if (length >= 0) {
          cIptvStreamerStatistics::AddStatistic(length);
          mutex->Lock();
          int p = ringBuffer->Put(buffer, length);
          if (p != length && Running())
             ringBuffer->ReportOverflow(length - p);
          mutex->Unlock();
	  cIptvBufferStatistics::AddStatistic(ringBuffer->Available(),
					      ringBuffer->Free());
          }
       else
          cCondWait::SleepMs(100); // to reduce cpu load
       }
    else
       cCondWait::SleepMs(100); // and avoid busy loop
    }
  debug("cIptvStreamer::Action(): Exiting\n");
}

bool cIptvStreamer::Open(void)
{
  debug("cIptvStreamer::Open()\n");
  // Open the protocol
  if (protocol)
     if(!protocol->Open())
       return false;
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

cString cIptvStreamer::GetInformation(void)
{
  //debug("cIptvStreamer::GetInformation()");
  if (protocol)
     return protocol->GetInformation();
  return NULL;
}
