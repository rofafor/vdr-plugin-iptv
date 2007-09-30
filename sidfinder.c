/*
 * sidfilter.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: sidfinder.c,v 1.3 2007/09/30 17:33:02 ajhseppa Exp $
 */

#include <libsi/section.h>

#include "common.h"
#include "sidfinder.h"

cSidFinder::cSidFinder(void)
{
  debug("cSidFinder::cSidFinder()\n");
  channel = cChannel();
  Set(0x00, 0x00);  // PAT
}

void cSidFinder::SetStatus(bool On)
{
  debug("cSidFinder::SetStatus(): %d\n", On);
  cFilter::SetStatus(On);
}

void cSidFinder::SetChannel(const cChannel *Channel)
{
  if (Channel) {
     debug("cSidFinder::SetChannel(): %s\n", Channel->PluginParam());
     channel = *Channel;
     }
  else {
     debug("cSidFinder::SetChannel()\n");
     channel = cChannel();
     }
}

void cSidFinder::Process(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  //debug("cSidFinder::Process()\n");
  if ((Pid == 0x00) && (Tid == 0x00) && channel.GetChannelID().Valid()) {
     debug("cSidFinder::Process(): Pid=%d Tid=%02X\n", Pid, Tid);
     SI::PAT pat(Data, false);
     if (!pat.CheckCRCAndParse())
        return;
     SI::PAT::Association assoc;
     for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); ) {
         if (!assoc.isNITPid()) {
            if (assoc.getServiceId() != channel.Sid()) {
               debug("cSidFinder::Process(): Sid=%d\n", assoc.getServiceId());
               if (!Channels.Lock(true, 10))
                  return;
               cChannel *IptvChannel = Channels.GetByChannelID(channel.GetChannelID());
               IptvChannel->SetId(IptvChannel->Nid(), IptvChannel->Tid(), assoc.getServiceId(), IptvChannel->Rid());
               Channels.Unlock();
               }
            SetChannel(NULL);
            SetStatus(false);
            return;
            }
         }
     }
}

