/*
 * sidscanner.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <libsi/section.h>

#include "common.h"
#include "sidscanner.h"

cSidScanner::cSidScanner(void)
: channelId(tChannelID::InvalidID),
  sidFound(false),
  nidFound(false),
  tidFound(false)
{
  debug("cSidScanner::cSidScanner()\n");
  Set(0x00, 0x00);  // PAT
  Set(0x10, 0x40);  // NIT
}

cSidScanner::~cSidScanner()
{
  debug("cSidScanner::~cSidScanner()\n");
}

void cSidScanner::SetStatus(bool On)
{
  debug("cSidScanner::SetStatus(): %d\n", On);
  cFilter::SetStatus(On);
}

void cSidScanner::SetChannel(const tChannelID &ChannelId)
{
  debug("cSidScanner::SetChannel(): %s\n", *ChannelId.ToString());
  channelId = ChannelId;
  sidFound = false;
  nidFound = false;
  tidFound = false;
}

void cSidScanner::Process(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  int newSid = -1, newNid = -1, newTid = -1;

  //debug("cSidScanner::Process()\n");
  if (channelId.Valid()) {
     if ((Pid == 0x00) && (Tid == 0x00)) {
        debug("cSidScanner::Process(): Pid=%d Tid=%02X\n", Pid, Tid);
        SI::PAT pat(Data, false);
        if (!pat.CheckCRCAndParse())
           return;
        SI::PAT::Association assoc;
        for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); ) {
            if (!assoc.isNITPid()) {
               if (assoc.getServiceId() != channelId.Sid()) {
                  debug("cSidScanner::Process(): Sid=%d\n", assoc.getServiceId());
                  newSid = assoc.getServiceId();
                  }
               sidFound = true;
               break;
               }
            }
        }
     else if ((Pid == 0x10) && (Tid == 0x40)) {
        debug("cSidScanner::Process(): Pid=%d Tid=%02X\n", Pid, Tid);
        SI::NIT nit(Data, false);
        if (!nit.CheckCRCAndParse())
           return;
        SI::NIT::TransportStream ts;
        for (SI::Loop::Iterator it; nit.transportStreamLoop.getNext(ts, it); ) {
            if (ts.getTransportStreamId() != channelId.Tid()) {
               debug("cSidScanner::Process(): TSid=%d\n", ts.getTransportStreamId());
               newTid = ts.getTransportStreamId();
               }
            tidFound = true;
            break; // default to the first one
            }
        if (nit.getNetworkId() != channelId.Nid()) {
           debug("cSidScanner::Process(): Nid=%d\n", ts.getTransportStreamId());
           newNid = nit.getNetworkId();
           }
        nidFound = true; 
        }
     }
  if ((newSid >= 0) || (newNid >= 0) || (newTid >= 0)) {
     if (!Channels.Lock(true, 10))
        return;
     cChannel *IptvChannel = Channels.GetByChannelID(channelId);
     if (IptvChannel)
        IptvChannel->SetId((newNid < 0) ? IptvChannel->Nid() : newNid, (newTid < 0) ? IptvChannel->Tid() : newTid,
                           (newSid < 0) ? IptvChannel->Sid() : newSid, IptvChannel->Rid());
     Channels.Unlock();
     }
  if (sidFound && nidFound && tidFound) {
     SetChannel(tChannelID::InvalidID);
     SetStatus(false);
     }
}
