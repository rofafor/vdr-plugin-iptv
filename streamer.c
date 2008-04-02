/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: streamer.c,v 1.32 2008/04/02 22:55:04 rahrenbe Exp $
 */

#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

#include "common.h"
#include "streamer.h"

cIptvStreamer::cIptvStreamer(cRingBufferLinear* RingBuffer, cMutex* Mutex)
: cThread("IPTV streamer"),
  ringBuffer(RingBuffer),
  mutex(Mutex),
  protocol(NULL),
  location(""),
  parameter(-1),
  index(-1)
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
          AddStreamerStatistic(length);
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
  if (protocol && !protocol->Open())
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
  // reset stream variables
  protocol = NULL;
  location = cString("");
  parameter = -1;
  index = -1;

  return true;
}

bool cIptvStreamer::Set(const char* Location, const int Parameter, const int Index, cIptvProtocolIf* Protocol)
{
  debug("cIptvStreamer::Set(): %s:%d\n", Location, Parameter);
  if (!isempty(Location)) {
     // Check if (re)tune is needed
     //if ((strcmp(*location, Location) == 0) && (parameter == Parameter) && (index == Index) && (protocol == Protocol)) {
     //   debug("cIptvStreamer::Set(): (Re)tune skipped\n");
     //   return false;
     //   }
     // Update protocol; Close the existing one if changed
     if (protocol != Protocol) {
        if (protocol)
           protocol->Close();
        protocol = Protocol;
        if (protocol)
           protocol->Open();
        }
     // Set protocol location and parameter
     if (protocol) {
        location = cString(Location);
        parameter = Parameter;
        index = Index;
        protocol->Set(location, parameter, index);
        }
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
