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
  debug("cSidScanner::%s()", __FUNCTION__);
  Set(0x00, 0x00);  // PAT
  Set(0x10, 0x40);  // NIT
}

cSidScanner::~cSidScanner()
{
  debug("cSidScanner::%s()", __FUNCTION__);
}

void cSidScanner::SetChannel(const tChannelID &channelIdP)
{
  debug("cSidScanner::%s(%s)", __FUNCTION__, *channelIdP.ToString());
  channelIdM = channelIdP;
  sidFoundM = false;
  nidFoundM = false;
  tidFoundM = false;
}

void cSidScanner::Process(u_short pidP, u_char tidP, const u_char *dataP, int lengthP)
{
  int newSid = -1, newNid = -1, newTid = -1;

  //debug("cSidScanner::%s()", __FUNCTION__);
  if (!isActiveM)
     return;
  if (channelIdM.Valid()) {
     if ((pidP == 0x00) && (tidP == 0x00)) {
        debug("cSidScanner::%s(%d, %02X)", __FUNCTION__, pidP, tidP);
        SI::PAT pat(dataP, false);
        if (!pat.CheckCRCAndParse())
           return;
        SI::PAT::Association assoc;
        for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); ) {
            if (!assoc.isNITPid()) {
               if (assoc.getServiceId() != channelIdM.Sid()) {
                  debug("cSidScanner::%s(): sid=%d", __FUNCTION__, assoc.getServiceId());
                  newSid = assoc.getServiceId();
                  }
               sidFoundM = true;
               break;
               }
            }
        }
     else if ((pidP == 0x10) && (tidP == 0x40)) {
        debug("cSidScanner::%s(%d, %02X)", __FUNCTION__, pidP, tidP);
        SI::NIT nit(dataP, false);
        if (!nit.CheckCRCAndParse())
           return;
        SI::NIT::TransportStream ts;
        for (SI::Loop::Iterator it; nit.transportStreamLoop.getNext(ts, it); ) {
            if (ts.getTransportStreamId() != channelIdM.Tid()) {
               debug("cSidScanner::%s(): tsid=%d", __FUNCTION__, ts.getTransportStreamId());
               newTid = ts.getTransportStreamId();
               }
            tidFoundM = true;
            break; // default to the first one
            }
        if (nit.getNetworkId() != channelIdM.Nid()) {
           debug("cSidScanner::%s(): nid=%d", __FUNCTION__, ts.getTransportStreamId());
           newNid = nit.getNetworkId();
           }
        nidFoundM = true;
        }
     }
  if ((newSid >= 0) || (newNid >= 0) || (newTid >= 0)) {
     if (!Channels.Lock(true, 10))
        return;
     cChannel *IptvChannel = Channels.GetByChannelID(channelIdM);
     if (IptvChannel)
        IptvChannel->SetId((newNid < 0) ? IptvChannel->Nid() : newNid, (newTid < 0) ? IptvChannel->Tid() : newTid,
                           (newSid < 0) ? IptvChannel->Sid() : newSid, IptvChannel->Rid());
     Channels.Unlock();
     }
  if (sidFoundM && nidFoundM && tidFoundM) {
     SetChannel(tChannelID::InvalidID);
     SetStatus(false);
     }
}
