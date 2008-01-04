/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.c,v 1.28 2008/01/04 23:36:37 ajhseppa Exp $
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
       mutex->Lock();
       int length = protocol->Read(&buffer);
       if (length >= 0) {
          AddStatistic(length);
          int p = ringBuffer->Put(buffer, length);
          if (p != length && Running())
             ringBuffer->ReportOverflow(length - p);
          mutex->Unlock();
          }
       else {
          mutex->Unlock();
          cCondWait::SleepMs(100); // to reduce cpu load
          }
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
  // Close the protocol. A mutex should be taken here to avoid a race condition
  // where thread Action() may be in the process of accessing the protocol.
  // Taking a mutex serializes the Close() and Action() -calls.
  if (mutex)
      mutex->Lock();
  if (protocol)
     protocol->Close();
  if (mutex)
     mutex->Unlock();

  return true;
}

bool cIptvStreamer::Set(const char* Location, const int Parameter, const int Index, cIptvProtocolIf* Protocol)
{
  debug("cIptvStreamer::Set(): %s:%d\n", Location, Parameter);
  if (!isempty(Location)) {
     // Update protocol; Close the existing one if changed
     if (protocol != Protocol) {
        if (protocol)
           protocol->Close();
        protocol = Protocol;
        if (protocol)
           protocol->Open();
        }
     // Set protocol location and parameter
     if (protocol)
        protocol->Set(Location, Parameter, Index);
     }
  return true;
}

cString cIptvStreamer::GetInformation(void)
{
  //debug("cIptvStreamer::GetInformation()");
  cString info("Stream:");
  if (protocol)
     info = cString::sprintf("%s %s", *info, *protocol->GetInformation());
  return cString::sprintf("%s\n", *info);
}
