/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

#include "common.h"
#include "streamer.h"

cIptvStreamer::cIptvStreamer(cRingBufferLinear* RingBuffer, unsigned int PacketLen)
: cThread("IPTV streamer"),
  ringBuffer(RingBuffer),
  packetBufferLen(PacketLen),
  protocol(NULL)
{
  debug("cIptvStreamer::cIptvStreamer(%d)\n", packetBufferLen);
  // Allocate packet buffer
  packetBuffer = MALLOC(unsigned char, packetBufferLen);
  if (packetBuffer)
     memset(packetBuffer, 0, packetBufferLen);
  else
     error("MALLOC() failed for packet buffer");
}

cIptvStreamer::~cIptvStreamer()
{
  debug("cIptvStreamer::~cIptvStreamer()\n");
  // Close the protocol
  Close();
  protocol = NULL;
  ringBuffer = NULL;
  // Free allocated memory
  free(packetBuffer);
}

void cIptvStreamer::Action(void)
{
  debug("cIptvStreamer::Action(): Entering\n");
  // Increase priority
  //SetPriority(-1);
  // Do the thread loop
  while (packetBuffer && Running()) {
        int length = -1;
        if (protocol)
           length = protocol->Read(packetBuffer, packetBufferLen);
        if (length > 0) {
           AddStreamerStatistic(length);
           if (ringBuffer) {
              int p = ringBuffer->Put(packetBuffer, length);
              if (p != length)
                 ringBuffer->ReportOverflow(length - p);
              }
           }
        else
           sleep.Wait(10); // to avoid busy loop and reduce cpu load
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
  sleep.Signal();
  if (Running())
     Cancel(3);
  // Close the protocol
  if (protocol)
     protocol->Close();
  return true;
}

bool cIptvStreamer::Set(const char* Location, const int Parameter, const int Index, cIptvProtocolIf* Protocol)
{
  debug("cIptvStreamer::Set(): %s:%d\n", Location, Parameter);
  if (!isempty(Location)) {
     // Update protocol and set location and parameter; Close the existing one if changed
     if (protocol != Protocol) {
        if (protocol)
           protocol->Close();
        protocol = Protocol;
        if (protocol) {
           protocol->Set(Location, Parameter, Index);
           protocol->Open();
           }
        }
     else if (protocol)
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
