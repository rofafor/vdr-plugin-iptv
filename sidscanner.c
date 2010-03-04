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
{
  debug("cSidScanner::cSidScanner()\n");
  channel = cChannel();
  Set(0x00, 0x00);  // PAT
}

void cSidScanner::SetStatus(bool On)
{
  debug("cSidScanner::SetStatus(): %d\n", On);
  cFilter::SetStatus(On);
}

void cSidScanner::SetChannel(const cChannel *Channel)
{
  if (Channel) {
     debug("cSidScanner::SetChannel(): %s\n", Channel->Parameters());
     channel = *Channel;
     }
  else {
     debug("cSidScanner::SetChannel()\n");
     channel = cChannel();
     }
}

void cSidScanner::Process(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  //debug("cSidScanner::Process()\n");
  if ((Pid == 0x00) && (Tid == 0x00) && channel.GetChannelID().Valid()) {
     debug("cSidScanner::Process(): Pid=%d Tid=%02X\n", Pid, Tid);
     SI::PAT pat(Data, false);
     if (!pat.CheckCRCAndParse())
        return;
     SI::PAT::Association assoc;
     for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); ) {
         if (!assoc.isNITPid()) {
            if (assoc.getServiceId() != channel.Sid()) {
               debug("cSidScanner::Process(): Sid=%d\n", assoc.getServiceId());
               if (!Channels.Lock(true, 10))
                  return;
               cChannel *IptvChannel = Channels.GetByChannelID(channel.GetChannelID());
               if (IptvChannel)
                  IptvChannel->SetId(IptvChannel->Nid(), IptvChannel->Tid(),
                                    assoc.getServiceId(), IptvChannel->Rid());
               Channels.Unlock();
               }
            SetChannel(NULL);
            SetStatus(false);
            return;
            }
         }
     }
}
