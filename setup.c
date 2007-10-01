/*
 * setup.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: setup.c,v 1.19 2007/10/01 18:14:57 rahrenbe Exp $
 */

#include <string.h>

#include <vdr/device.h>
#include <vdr/interface.h>

#include "common.h"
#include "config.h"
#include "setup.h"

#ifndef trVDR
#define trVDR(s) tr(s)
#endif

// --- cIptvMenuEditChannel --------------------------------------------------

class cIptvMenuEditChannel : public cOsdMenu
{
private:
  enum {
    eProtocolUDP,
    eProtocolHTTP,
    eProtocolFILE,
    eProtocolCount
  };
  struct tIptvChannel {
    int frequency, source, protocol, port, vpid, ppid, tpid, sid, nid, tid, rid;
    int apid[MAXAPIDS + 1], dpid[MAXDPIDS + 1], spid[MAXSPIDS + 1], caids[MAXCAIDS + 1];
    char name[256], location[256];
  } data;
  cChannel *channel;
  const char *protocols[eProtocolCount];
  void Setup(void);
  cString GetIptvSettings(const char *Param, int *Port, int *Protocol);
  void GetChannelData(cChannel *Channel);
  void SetChannelData(cChannel *Channel);

public:
  cIptvMenuEditChannel(cChannel *Channel, bool New = false);
  virtual eOSState ProcessKey(eKeys Key);
};

cIptvMenuEditChannel::cIptvMenuEditChannel(cChannel *Channel, bool New)
:cOsdMenu(trVDR("Edit channel"), 16)
{
  protocols[eProtocolUDP]  = tr("UDP");
  protocols[eProtocolHTTP] = tr("HTTP");
  protocols[eProtocolFILE] = tr("FILE");
  channel = Channel;
  GetChannelData(channel);
  if (New) {
     channel = NULL;
     data.nid = 0;
     data.tid = 0;
     data.rid = 0;
     }
  Setup();
}

cString cIptvMenuEditChannel::GetIptvSettings(const char *Param, int *Port, int *Protocol)
{
  char *loc = NULL;
  if (sscanf(Param, "IPTV|UDP|%a[^|]|%u", &loc, Port) == 2) {
     cString addr(loc, true);
     *Protocol = eProtocolUDP;
     return addr;
     }
  else if (sscanf(Param, "IPTV|HTTP|%a[^|]|%u", &loc, Port) == 2) {
     cString addr(loc, true);
     *Protocol = eProtocolHTTP;
     return addr;
     }
  else if (sscanf(Param, "IPTV|FILE|%a[^|]|%u", &loc, Port) == 2) {
     cString addr(loc, true);
     *Protocol = eProtocolFILE;
     return addr;
     }
  return NULL;
}

void cIptvMenuEditChannel::GetChannelData(cChannel *Channel)
{
  if (Channel) {
     int port, protocol;
     data.frequency = Channel->Frequency();
     data.source = Channel->Source();
     data.vpid = Channel->Vpid();
     data.ppid = Channel->Ppid();
     data.tpid = Channel->Tpid();
     for (unsigned int i = 0; i < sizeof(data.apid); ++i)
         data.apid[i] = Channel->Apid(i);
     for (unsigned int i = 0; i < sizeof(data.dpid); ++i)
         data.dpid[i] = Channel->Dpid(i);
     for (unsigned int i = 0; i < sizeof(data.spid); ++i)
         data.spid[i] = Channel->Spid(i);
     for (unsigned int i = 0; i < sizeof(data.caids); ++i)
         data.caids[i] = Channel->Ca(i);
     data.sid = Channel->Sid();
     data.nid = Channel->Nid();
     data.tid = Channel->Tid();
     data.rid = Channel->Rid();
     strn0cpy(data.name, Channel->Name(), sizeof(data.name));
     strn0cpy(data.location, *GetIptvSettings(Channel->PluginParam(), &port, &protocol), sizeof(data.location));
     data.protocol = protocol;
     data.port = port;
     }
  else {
     data.frequency = 1;
     data.source = cSource::FromData(cSource::stPlug);
     data.vpid = 0;
     data.ppid = 0;
     data.tpid = 0;
     for (unsigned int i = 0; i < sizeof(data.apid); ++i)
         data.apid[i] = 0;
     for (unsigned int i = 0; i < sizeof(data.dpid); ++i)
         data.dpid[i] = 0;
     for (unsigned int i = 0; i < sizeof(data.spid); ++i)
         data.spid[i] = 0;
     for (unsigned int i = 0; i < sizeof(data.caids); ++i)
         data.caids[i] = 0;
     data.sid = 1;
     data.nid = 0;
     data.tid = 0;
     data.rid = 0;
     strn0cpy(data.name, "IPTV", sizeof(data.name));
     strn0cpy(data.location, "127.0.0.1", sizeof(data.location));
     data.protocol = eProtocolUDP;
     data.port = 1234;
     }
}

void cIptvMenuEditChannel::SetChannelData(cChannel *Channel)
{
  if (Channel) {
     cString param;
     char alangs[MAXAPIDS][MAXLANGCODE2] = { "" };
     char dlangs[MAXDPIDS][MAXLANGCODE2] = { "" };
     switch (data.protocol) {
       case eProtocolFILE:
            param = cString::sprintf("IPTV|FILE|%s|%d", data.location, data.port);
            break;
       case eProtocolHTTP:
            param = cString::sprintf("IPTV|HTTP|%s|%d", data.location, data.port);
            break;
       default:
       case eProtocolUDP:
            param = cString::sprintf("IPTV|UDP|%s|%d", data.location, data.port);
            break;
       }
     Channel->SetPids(data.vpid, data.ppid, data.apid, alangs, data.dpid, dlangs, data.tpid);
     Channel->SetCaIds(data.caids);
     Channel->SetId(data.nid, data.tid, data.sid, data.rid);
     Channel->SetName(data.name, "", "IPTV");
     Channel->SetPlugTransponderData(cSource::stPlug, data.frequency, param);
     }
}

void cIptvMenuEditChannel::Setup(void)
{
  int current = Current();
  Clear();
  // IPTV specific settings
  Add(new cMenuEditStraItem(tr("Protocol"),    &data.protocol, 3, protocols));
  switch (data.protocol) {
    case eProtocolFILE:
         Add(new cMenuEditStrItem(trVDR("File"),     data.location, sizeof(data.location), trVDR(FileNameChars)));
         Add(new cMenuEditIntItem(tr("Delay (ms)"), &data.port,  0, 0xFFFF));
         break;
    case eProtocolHTTP:
    case eProtocolUDP:
    default:
         Add(new cMenuEditStrItem(tr("Address"), data.location, sizeof(data.location), trVDR(FileNameChars)));
         Add(new cMenuEditIntItem(tr("Port"),   &data.port,  0, 0xFFFF));
         break;
    }
  // Normal settings
  Add(new cMenuEditStrItem(trVDR("Name"),       data.name,     sizeof(data.name), trVDR(FileNameChars)));
  Add(new cMenuEditIntItem(trVDR("Frequency"), &data.frequency));
  Add(new cMenuEditIntItem(trVDR("Vpid"),      &data.vpid,     0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Ppid"),      &data.ppid,     0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Apid1"),     &data.apid[0],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Apid2"),     &data.apid[1],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Dpid1"),     &data.dpid[0],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Dpid2"),     &data.dpid[1],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Tpid"),      &data.tpid,     0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("CA"),        &data.caids[0], 0, 0xFFFF));
  Add(new cMenuEditIntItem(trVDR("Sid"),       &data.sid,      1, 0xFFFF));
  Add(new cMenuEditIntItem(tr   ("Rid"),       &data.rid,      0, 0x1FFF));
  SetCurrent(Get(current));
  Display();
}

eOSState cIptvMenuEditChannel::ProcessKey(eKeys Key)
{
  int oldProtocol = data.protocol;
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
     if (Key == kOk) {
        cChannel newchannel;
        SetChannelData(&newchannel);
        bool uniquityFailed = false;
        bool firstIncrement = true;
        // Search for identical channels as these will be ignored by vdr
        for (cChannel *iteratorChannel = Channels.First(); iteratorChannel;
             iteratorChannel = Channels.Next(iteratorChannel)) {
           // This is one of the channels cause the uniquity check to fail
           if (!iteratorChannel->GroupSep() && iteratorChannel != channel
               && iteratorChannel->GetChannelID() == newchannel.GetChannelID()) {
              // See if it has unique Plugin param. If yes then increment
              // the corresponding Rid until it is unique
              if (strcmp(iteratorChannel->PluginParam(),
                         newchannel.PluginParam())) {
                 // If the channel RID is already at maximum, then fail the
                 // channel modification
                 if (iteratorChannel->Rid() >= 0x1FFF) {
                    debug("Cannot increment RID over maximum value\n");
                    uniquityFailed = true;
                    break;
                    }
                 debug("Incrementing conflicting channel RID\n");
                 iteratorChannel->SetId(iteratorChannel->Nid(),
                                        iteratorChannel->Tid(),
                                        iteratorChannel->Sid(),
                                        firstIncrement ?
                                        0 : iteratorChannel->Rid() + 1);

                 // Try zero Rid:s at first increment. Prevents them from
                 // creeping slowly towards their maximum value
                 firstIncrement = false;

                 // Re-set the search and start again
                 iteratorChannel = Channels.First();
                 continue;
                 // Cannot work around by incrementing rid because channels
                 // are actually copies of each other
                 }
              else {
                 uniquityFailed = true;
                 break;
                 }
              }
           }
        if (!uniquityFailed) {
           if (channel) {
              SetChannelData(channel);
              isyslog("edited channel %d %s", channel->Number(), *channel->ToText());
              state = osBack;
              }
           else {
              channel = new cChannel;
              SetChannelData(channel);
              Channels.Add(channel);
              Channels.ReNumber();
              isyslog("added channel %d %s", channel->Number(), *channel->ToText());
              state = osUser1;
              }
           Channels.SetModified(true);
           }     
        else {
           Skins.Message(mtError, tr("Cannot find unique channel settings!"));
           state = osContinue;
           }
        }
     }
  if ((Key != kNone) && (data.protocol != oldProtocol)) {
     switch (data.protocol) {
       case eProtocolFILE:
            strn0cpy(data.location, "/tmp/video.ts", sizeof(data.location));
            data.port = 0;
            break;
       case eProtocolHTTP:
            strn0cpy(data.location, "127.0.0.1/TS/1", sizeof(data.location));
            data.port = 3000;
            break;
       default:
       case eProtocolUDP:
            strn0cpy(data.location, "127.0.0.1", sizeof(data.location));
            data.port = 1234;
            break;
       }
     Setup();
     }
  return state;
}

// --- cIptvMenuChannelItem --------------------------------------------------

class cIptvMenuChannelItem : public cOsdItem
{
private:
  cChannel *channel;

public:
  cIptvMenuChannelItem(cChannel *Channel);
  virtual void Set(void);
  cChannel *Channel(void) { return channel; }
};

cIptvMenuChannelItem::cIptvMenuChannelItem(cChannel *Channel)
{
  channel = Channel;
  Set();
}

void cIptvMenuChannelItem::Set(void)
{
  char *buffer = NULL;
  asprintf(&buffer, "%d\t%s", channel->Number(), channel->Name());
  SetText(buffer, false);
}

// --- cIptvMenuChannels -----------------------------------------------------

class cIptvMenuChannels : public cOsdMenu
{
private:
  void Setup(void);
  cChannel *GetChannel(int Index);
  void Propagate(void);

protected:
  eOSState Edit(void);
  eOSState New(void);
  eOSState Delete(void);
  eOSState Switch(void);

public:
  cIptvMenuChannels();
  ~cIptvMenuChannels();
  virtual eOSState ProcessKey(eKeys Key);
};

cIptvMenuChannels::cIptvMenuChannels(void)
:cOsdMenu(tr("IPTV Channels"), numdigits(Channels.MaxNumber()) + 1)
{
  Setup();
  Channels.IncBeingEdited();
}

cIptvMenuChannels::~cIptvMenuChannels()
{
  Channels.DecBeingEdited();
}

void cIptvMenuChannels::Setup(void)
{
  Clear();
  for (cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel)) {
      if (!channel->GroupSep() && channel->IsPlug() && !strncmp(channel->PluginParam(), "IPTV", 4)) {
         cIptvMenuChannelItem *item = new cIptvMenuChannelItem(channel);
         Add(item);
         }
      }
  SetHelp(trVDR("Button$Edit"), trVDR("Button$New"), trVDR("Button$Delete"), NULL);
  Display();
}

cChannel *cIptvMenuChannels::GetChannel(int Index)
{
  cIptvMenuChannelItem *p = (cIptvMenuChannelItem *)Get(Index);
  return p ? (cChannel *)p->Channel() : NULL;
}

void cIptvMenuChannels::Propagate(void)
{
  Channels.ReNumber();
  for (cIptvMenuChannelItem *ci = (cIptvMenuChannelItem *)First(); ci; ci = (cIptvMenuChannelItem *)ci->Next())
      ci->Set();
  Display();
  Channels.SetModified(true);
}

eOSState cIptvMenuChannels::Switch(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cChannel *ch = GetChannel(Current());
  if (ch)
     return cDevice::PrimaryDevice()->SwitchChannel(ch, true) ? osEnd : osContinue;
  return osEnd;
}

eOSState cIptvMenuChannels::Edit(void)
{
  if (HasSubMenu() || Count() == 0)
     return osContinue;
  cChannel *ch = GetChannel(Current());
  if (ch)
     return AddSubMenu(new cIptvMenuEditChannel(ch));
  return osContinue;
}

eOSState cIptvMenuChannels::New(void)
{
  if (HasSubMenu())
     return osContinue;
  return AddSubMenu(new cIptvMenuEditChannel(GetChannel(Current()), true));
}

eOSState cIptvMenuChannels::Delete(void)
{
  if (!HasSubMenu() && Count() > 0) {
     int CurrentChannelNr = cDevice::CurrentChannel();
     cChannel *CurrentChannel = Channels.GetByNumber(CurrentChannelNr);
     int Index = Current();
     cChannel *channel = GetChannel(Current());
     int DeletedChannel = channel->Number();
     // Check if there is a timer using this channel:
     if (channel->HasTimer()) {
        Skins.Message(mtError, trVDR("Channel is being used by a timer!"));
        return osContinue;
        }
     if (Interface->Confirm(trVDR("Delete channel?"))) {
        if (CurrentChannel && channel == CurrentChannel) {
           int n = Channels.GetNextNormal(CurrentChannel->Index());
           if (n < 0)
              n = Channels.GetPrevNormal(CurrentChannel->Index());
           CurrentChannel = Channels.Get(n);
           CurrentChannelNr = 0; // triggers channel switch below
           }
        Channels.Del(channel);
        cOsdMenu::Del(Index);
        Propagate();
        isyslog("channel %d deleted", DeletedChannel);
        if (CurrentChannel && CurrentChannel->Number() != CurrentChannelNr) {
           if (!cDevice::PrimaryDevice()->Replaying() || cDevice::PrimaryDevice()->Transferring())
              Channels.SwitchTo(CurrentChannel->Number());
           else
              cDevice::SetCurrentChannel(CurrentChannel);
           }
        }
     }
  return osContinue;
}

eOSState cIptvMenuChannels::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  switch (state) {
    case osUser1: {
         cChannel *channel = Channels.Last();
         if (channel) {
            Add(new cIptvMenuChannelItem(channel), true);
            return CloseSubMenu();
            }
         }
         break;
    default:
         if (state == osUnknown) {
            switch (Key) {
              case kOk:     return Switch();
              case kRed:    return Edit();
              case kGreen:  return New();
              case kYellow: return Delete();
              default: break;
              }
            }
    }
  return state;
}

// --- cIptvPluginSetup ------------------------------------------------------

cIptvPluginSetup::cIptvPluginSetup()
{
  tsBufferSize = IptvConfig.GetTsBufferSize();
  tsBufferPrefill = IptvConfig.GetTsBufferPrefillRatio();
  sectionFiltering = IptvConfig.GetSectionFiltering();
  sidScanning = IptvConfig.GetSidScanning();
  Setup();
  SetHelp(trVDR("Channels"), NULL, NULL, NULL);
}

void cIptvPluginSetup::Setup(void)
{
  int current = Current();
  Clear();
  Add(new cMenuEditIntItem( tr("TS buffer size [MB]"),         &tsBufferSize,     2, 16));
  Add(new cMenuEditIntItem( tr("TS buffer prefill ratio [%]"), &tsBufferPrefill,  0, 40));
  Add(new cMenuEditBoolItem(tr("Use section filtering"),       &sectionFiltering));  
  if (sectionFiltering)
     Add(new cMenuEditBoolItem(tr("Scan Sid automatically"),   &sidScanning));
  SetCurrent(Get(current));
  Display();
}

eOSState cIptvPluginSetup::EditChannel(void)
{
  if (HasSubMenu())
     return osContinue;
  return AddSubMenu(new cIptvMenuChannels());
}

eOSState cIptvPluginSetup::ProcessKey(eKeys Key)
{
  eOSState state = cMenuSetupPage::ProcessKey(Key);
  if (state == osUnknown) {
     switch (Key) {
       case kRed: return EditChannel();
       default:   break;
       }
     }
  return state;
}

void cIptvPluginSetup::Store(void)
{
  // Store values into setup.conf
  SetupStore("TsBufferSize", tsBufferSize);
  SetupStore("TsBufferPrefill", tsBufferPrefill);
  SetupStore("SectionFiltering", sectionFiltering);
  SetupStore("SidScanning", sidScanning);
  // Update global config
  IptvConfig.SetTsBufferSize(tsBufferSize);
  IptvConfig.SetTsBufferPrefillRatio(tsBufferPrefill);
  IptvConfig.SetSectionFiltering(sectionFiltering);
  IptvConfig.SetSidScanning(sidScanning);
}
