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

cIptvStreamer::cIptvStreamer(cRingBufferLinear* ringBufferP, unsigned int packetLenP)
: cThread("IPTV streamer"),
  ringBufferM(ringBufferP),
  sleepM(),
  packetBufferLenM(packetLenP),
  protocolM(NULL)
{
  debug("cIptvStreamer::%s(%d)", __FUNCTION__, packetBufferLenM);
  // Allocate packet buffer
  packetBufferM = MALLOC(unsigned char, packetBufferLenM);
  if (packetBufferM)
     memset(packetBufferM, 0, packetBufferLenM);
  else
     error("MALLOC() failed for packet buffer");
}

cIptvStreamer::~cIptvStreamer()
{
  debug("cIptvStreamer::%s()", __FUNCTION__);
  // Close the protocol
  Close();
  protocolM = NULL;
  ringBufferM = NULL;
  // Free allocated memory
  free(packetBufferM);
}

void cIptvStreamer::Action(void)
{
  debug("cIptvStreamer::%s(): entering", __FUNCTION__);
  // Increase priority
  //SetPriority(-1);
  // Do the thread loop
  while (packetBufferM && Running()) {
        int length = -1;
        unsigned int size = min((unsigned int)ringBufferM->Free(), packetBufferLenM);
        if (protocolM && (size > 0))
           length = protocolM->Read(packetBufferM, size);
        if (length > 0) {
           AddStreamerStatistic(length);
           if (ringBufferM) {
              int p = ringBufferM->Put(packetBufferM, length);
              if (p != length)
                 ringBufferM->ReportOverflow(length - p);
              }
           }
        else
           sleepM.Wait(10); // to avoid busy loop and reduce cpu load
        }
  debug("cIptvStreamer::%s(): exiting", __FUNCTION__);
}

bool cIptvStreamer::Open(void)
{
  debug("cIptvStreamer::%s()", __FUNCTION__);
  // Open the protocol
  if (protocolM && !protocolM->Open())
     return false;
  // Start thread
  Start();
  return true;
}

bool cIptvStreamer::Close(void)
{
  debug("cIptvStreamer::%s()", __FUNCTION__);
  // Stop thread
  sleepM.Signal();
  if (Running())
     Cancel(3);
  // Close the protocol
  if (protocolM)
     protocolM->Close();
  return true;
}

bool cIptvStreamer::Set(const char* locationP, const int parameterP, const int indexP, cIptvProtocolIf* protocolP)
{
  debug("cIptvStreamer::%s(%s, %d, %d)", __FUNCTION__, locationP, parameterP, indexP);
  if (!isempty(locationP)) {
     // Update protocol and set location and parameter; Close the existing one if changed
     if (protocolM != protocolP) {
        if (protocolM)
           protocolM->Close();
        protocolM = protocolP;
        if (protocolM) {
           protocolM->Set(locationP, parameterP, indexP);
           protocolM->Open();
           }
        }
     else if (protocolM)
        protocolM->Set(locationP, parameterP, indexP);
     }
  return true;
}

cString cIptvStreamer::GetInformation(void)
{
  //debug("cIptvStreamer::%s()", __FUNCTION__);
  cString s;
  if (protocolM)
     s = protocolM->GetInformation();
  return s;
}
