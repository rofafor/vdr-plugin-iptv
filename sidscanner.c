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
: channelIdM(tChannelID::InvalidID),
  sidFoundM(false),
  nidFoundM(false),
  tidFoundM(false),
  isActiveM(false)
{
  debug1("%s", __PRETTY_FUNCTION__);
  Set(0x00, 0x00);  // PAT
  Set(0x10, 0x40);  // NIT
}

cSidScanner::~cSidScanner()
{
  debug1("%s", __PRETTY_FUNCTION__);
}

void cSidScanner::SetChannel(const tChannelID &channelIdP)
{
  debug1("%s (%s)", __PRETTY_FUNCTION__, *channelIdP.ToString());
  channelIdM = channelIdP;
  sidFoundM = false;
  nidFoundM = false;
  tidFoundM = false;
}

void cSidScanner::Process(u_short pidP, u_char tidP, const u_char *dataP, int lengthP)
{
  int newSid = -1, newNid = -1, newTid = -1;

  debug16("%s (%d, %02X, , %d)", __PRETTY_FUNCTION__, pidP, tidP, lengthP);
  if (!isActiveM)
     return;
  if (channelIdM.Valid()) {
     if ((pidP == 0x00) && (tidP == 0x00)) {
        debug16("%s (%d, %02X, , %d) pat", __PRETTY_FUNCTION__, pidP, tidP, lengthP);
        SI::PAT pat(dataP, false);
        if (!pat.CheckCRCAndParse())
           return;
        SI::PAT::Association assoc;
        for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); ) {
            if (!assoc.isNITPid()) {
               if (assoc.getServiceId() != channelIdM.Sid()) {
                  debug1("%s (%d, %02X, , %d) sid=%d", __PRETTY_FUNCTION__, pidP, tidP, lengthP, assoc.getServiceId());
                  newSid = assoc.getServiceId();
                  }
               sidFoundM = true;
               break;
               }
            }
        }
     else if ((pidP == 0x10) && (tidP == 0x40)) {
        debug1("%s (%d, %02X, , %d)", __PRETTY_FUNCTION__, pidP, tidP, lengthP);
        SI::NIT nit(dataP, false);
        if (!nit.CheckCRCAndParse())
           return;
        SI::NIT::TransportStream ts;
        for (SI::Loop::Iterator it; nit.transportStreamLoop.getNext(ts, it); ) {
            if (ts.getTransportStreamId() != channelIdM.Tid()) {
               debug1("%s (%d, %02X, , %d) tsid=%d", __PRETTY_FUNCTION__, pidP, tidP, lengthP, ts.getTransportStreamId());
               newTid = ts.getTransportStreamId();
               tidFoundM = true;
               }
            if (ts.getOriginalNetworkId() != channelIdM.Nid()) {
               debug1("%s (%d, %02X, , %d) onid=%d", __PRETTY_FUNCTION__, pidP, tidP, lengthP, ts.getOriginalNetworkId());
               newNid = ts.getOriginalNetworkId();
               nidFoundM = true;
               }
            break; // default to the first one
            }
        // fallback for network id if not found already
        if (!nidFoundM && (nit.getNetworkId() != channelIdM.Nid())) {
           debug1("%s (%d, %02X, , %d) nid=%d", __PRETTY_FUNCTION__, pidP, tidP, lengthP, nit.getNetworkId());
           newNid = nit.getNetworkId();
           nidFoundM = true;
           }
        }
     }
  if ((newSid >= 0) || (newNid >= 0) || (newTid >= 0)) {
     cStateKey StateKey;
     cChannels *Channels = cChannels::GetChannelsWrite(StateKey, 10);
     if (!Channels)
        return;
     bool ChannelsModified = false;
     cChannel *IptvChannel = Channels->GetByChannelID(channelIdM);
     if (IptvChannel)
        ChannelsModified |= IptvChannel->SetId(Channels, (newNid < 0) ? IptvChannel->Nid() : newNid,
                                               (newTid < 0) ? IptvChannel->Tid() : newTid,
                                               (newSid < 0) ? IptvChannel->Sid() : newSid, IptvChannel->Rid());
     StateKey.Remove(ChannelsModified);
     }
  if (sidFoundM && nidFoundM && tidFoundM) {
     SetChannel(tChannelID::InvalidID);
     SetStatus(false);
     }
}
