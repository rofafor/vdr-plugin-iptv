/*
 * streamer.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "log.h"
#include "streamer.h"

cIptvStreamer::cIptvStreamer(cIptvDeviceIf &deviceP, unsigned int packetLenP)
: cThread("IPTV streamer"),
  sleepM(),
  deviceM(&deviceP),
  packetBufferLenM(packetLenP),
  protocolM(NULL)
{
  debug1("%s (, %d)", __PRETTY_FUNCTION__, packetBufferLenM);
  // Allocate packet buffer
  packetBufferM = MALLOC(unsigned char, packetBufferLenM);
  if (packetBufferM)
     memset(packetBufferM, 0, packetBufferLenM);
  else
     error("MALLOC() failed for packet buffer");
}

cIptvStreamer::~cIptvStreamer()
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Close the protocol
  Close();
  protocolM = NULL;
  // Free allocated memory
  free(packetBufferM);
}

void cIptvStreamer::Action(void)
{
  debug1("%s() Entering", __PRETTY_FUNCTION__);
  // Increase priority
  //SetPriority(-1);
  // Do the thread loop
  while (packetBufferM && Running()) {
        int length = -1;
        unsigned int size = min(deviceM->CheckData(), packetBufferLenM);
        if (protocolM && (size > 0))
           length = protocolM->Read(packetBufferM, size);
        if (length > 0) {
           AddStreamerStatistic(length);
           deviceM->WriteData(packetBufferM, length);
           }
        else
           sleepM.Wait(10); // to avoid busy loop and reduce cpu load
        }
  debug1("%s Exiting", __PRETTY_FUNCTION__);
}

bool cIptvStreamer::Open(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Open the protocol
  if (protocolM && !protocolM->Open())
     return false;
  // Start thread
  Start();
  return true;
}

bool cIptvStreamer::Close(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Stop thread
  sleepM.Signal();
  if (Running())
     Cancel(3);
  // Close the protocol
  if (protocolM)
     protocolM->Close();
  return true;
}

bool cIptvStreamer::SetSource(const char* locationP, const int parameterP, const int indexP, cIptvProtocolIf* protocolP)
{
  debug1("%s (%s, %d, %d, )", __PRETTY_FUNCTION__, locationP, parameterP, indexP);
  if (!isempty(locationP)) {
     // Update protocol and set location and parameter; Close the existing one if changed
     if (protocolM != protocolP) {
        if (protocolM)
           protocolM->Close();
        protocolM = protocolP;
        if (protocolM) {
           protocolM->SetSource(locationP, parameterP, indexP);
           protocolM->Open();
           }
        }
     else if (protocolM)
        protocolM->SetSource(locationP, parameterP, indexP);
     }
  return true;
}

bool cIptvStreamer::SetPid(int pidP, int typeP, bool onP)
{
  debug1("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);
  if (protocolM)
     return protocolM->SetPid(pidP, typeP, onP);
  return true;
}

cString cIptvStreamer::GetInformation(void)
{
  debug16("%s", __PRETTY_FUNCTION__);
  cString s;
  if (protocolM)
     s = protocolM->GetInformation();
  return s;
}
