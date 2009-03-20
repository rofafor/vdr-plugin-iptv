/*
 * setup.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <string.h>

#include <vdr/device.h>
#include <vdr/interface.h>
#include <vdr/status.h>
#include <vdr/menu.h>

#include "common.h"
#include "config.h"
#include "device.h"
#include "setup.h"

// --- cIptvMenuEditChannel --------------------------------------------------

class cIptvMenuEditChannel : public cOsdMenu
{
private:
  enum {
    eProtocolUDP,
    eProtocolHTTP,
    eProtocolFILE,
    eProtocolEXT,
    eProtocolCount
  };
  struct tIptvChannel {
    int frequency, source, protocol, parameter, vpid, ppid, vtype, tpid, sid, nid, tid, rid;
    int apid[MAXAPIDS + 1], dpid[MAXDPIDS + 1], spid[MAXSPIDS + 1], caids[MAXCAIDS + 1];
    int sidscan, pidscan;
    char name[256], location[256];
  } data;
  cChannel *channel;
  const char *protocols[eProtocolCount];
  void Setup(void);
  cString GetIptvSettings(const char *Param, int *Parameter, int *SidScan, int *PidScan, int *Protocol);
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
  protocols[eProtocolEXT]  = tr("EXT");
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

cString cIptvMenuEditChannel::GetIptvSettings(const char *Param, int *Parameter, int *SidScan, int *PidScan, int *Protocol)
{
  char *tag = NULL;
  char *proto = NULL;
  char *loc = NULL;
  if (sscanf(Param, "%a[^|]|S%dP%d|%a[^|]|%a[^|]|%d", &tag, SidScan, PidScan, &proto, &loc, Parameter) == 6) {
     cString tagstr(tag, true);
     cString protostr(proto, true);
     cString locstr(loc, true);
     // check if IPTV tag
     if (strncasecmp(*tagstr, "IPTV", 4) == 0) {
        // check if protocol is supported and update the pointer
        if (strncasecmp(*protostr, "UDP", 3) == 0)
           *Protocol = eProtocolUDP;
        else if (strncasecmp(*protostr, "HTTP", 4) == 0)
           *Protocol = eProtocolHTTP;
        else if (strncasecmp(*protostr, "FILE", 4) == 0)
           *Protocol = eProtocolFILE;
        else if (strncasecmp(*protostr, "EXT", 3) == 0)
           *Protocol = eProtocolEXT;
        else
           return NULL;
        // return location
        return locstr;
        }
     }
  else if (sscanf(Param, "%a[^|]|P%dS%d|%a[^|]|%a[^|]|%d", &tag, PidScan, SidScan, &proto, &loc, Parameter) == 6) {
     cString tagstr(tag, true);
     cString protostr(proto, true);
     cString locstr(loc, true);
     // check if IPTV tag
     if (strncasecmp(*tagstr, "IPTV", 4) == 0) {
        // check if protocol is supported and update the pointer
        if (strncasecmp(*protostr, "UDP", 3) == 0)
           *Protocol = eProtocolUDP;
        else if (strncasecmp(*protostr, "HTTP", 4) == 0)
           *Protocol = eProtocolHTTP;
        else if (strncasecmp(*protostr, "FILE", 4) == 0)
           *Protocol = eProtocolFILE;
        else if (strncasecmp(*protostr, "EXT", 3) == 0)
           *Protocol = eProtocolEXT;
        else
           return NULL;
        // return location
        return locstr;
        }
     }
  return NULL;
}

void cIptvMenuEditChannel::GetChannelData(cChannel *Channel)
{
  if (Channel) {
     int parameter, protocol, sidscan, pidscan;
     data.frequency = Channel->Frequency();
     data.source = Channel->Source();
     data.vpid = Channel->Vpid();
     data.ppid = Channel->Ppid();
#if defined(APIVERSNUM) && APIVERSNUM >= 10704
     data.vtype = Channel->Vtype();
#endif
     data.tpid = Channel->Tpid();
     for (unsigned int i = 0; i < ARRAY_SIZE(data.apid); ++i)
         data.apid[i] = Channel->Apid(i);
     for (unsigned int i = 0; i < ARRAY_SIZE(data.dpid); ++i)
         data.dpid[i] = Channel->Dpid(i);
     for (unsigned int i = 0; i < ARRAY_SIZE(data.spid); ++i)
         data.spid[i] = Channel->Spid(i);
     for (unsigned int i = 0; i < ARRAY_SIZE(data.caids); ++i)
         data.caids[i] = Channel->Ca(i);
     data.sid = Channel->Sid();
     data.nid = Channel->Nid();
     data.tid = Channel->Tid();
     data.rid = Channel->Rid();
     strn0cpy(data.name, Channel->Name(), sizeof(data.name));
     strn0cpy(data.location, *GetIptvSettings(Channel->PluginParam(), &parameter, &sidscan, &pidscan, &protocol), sizeof(data.location));
     data.sidscan = sidscan;
     data.pidscan = pidscan;
     data.protocol = protocol;
     data.parameter = parameter;
     }
  else {
     data.frequency = 1;
     data.source = cSource::FromData(cSource::stPlug);
     data.vpid = 0;
     data.ppid = 0;
     data.vtype = 0;
     data.tpid = 0;
     for (unsigned int i = 0; i < ARRAY_SIZE(data.apid); ++i)
         data.apid[i] = 0;
     for (unsigned int i = 0; i < ARRAY_SIZE(data.dpid); ++i)
         data.dpid[i] = 0;
     for (unsigned int i = 0; i < ARRAY_SIZE(data.spid); ++i)
         data.spid[i] = 0;
     for (unsigned int i = 0; i < ARRAY_SIZE(data.caids); ++i)
         data.caids[i] = 0;
     data.sid = 1;
     data.nid = 0;
     data.tid = 0;
     data.rid = 0;
     strn0cpy(data.name, "IPTV", sizeof(data.name));
     strn0cpy(data.location, "127.0.0.1", sizeof(data.location));
     data.sidscan = 0;
     data.pidscan = 0;
     data.protocol = eProtocolUDP;
     data.parameter = 1234;
     }
}

void cIptvMenuEditChannel::SetChannelData(cChannel *Channel)
{
  if (Channel) {
     cString param;
     char alangs[MAXAPIDS][MAXLANGCODE2] = { "" };
     char dlangs[MAXDPIDS][MAXLANGCODE2] = { "" };
     switch (data.protocol) {
       case eProtocolEXT:
            param = cString::sprintf("IPTV|S%dP%d|EXT|%s|%d", data.sidscan, data.pidscan, data.location, data.parameter);
            break;
       case eProtocolFILE:
            param = cString::sprintf("IPTV|S%dP%d|FILE|%s|%d", data.sidscan, data.pidscan, data.location, data.parameter);
            break;
       case eProtocolHTTP:
            param = cString::sprintf("IPTV|S%dP%d|HTTP|%s|%d", data.sidscan, data.pidscan, data.location, data.parameter);
            break;
       default:
       case eProtocolUDP:
            param = cString::sprintf("IPTV|S%dP%d|UDP|%s|%d", data.sidscan, data.pidscan, data.location, data.parameter);
            break;
       }
     char slangs[MAXSPIDS][MAXLANGCODE2] = { "" };
#if defined(APIVERSNUM) && APIVERSNUM >= 10704
     Channel->SetPids(data.vpid, data.ppid, data.vtype, data.apid, alangs, data.dpid, dlangs, data.spid, slangs, data.tpid);
#else
     Channel->SetPids(data.vpid, data.ppid, data.apid, alangs, data.dpid, dlangs, data.spid, slangs, data.tpid);
#endif
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
  Add(new cMenuEditStraItem(tr("Protocol"),    &data.protocol,
                            eProtocolCount, protocols));
  switch (data.protocol) {
    case eProtocolFILE:
         Add(new cMenuEditStrItem(trVDR("File"),     data.location, sizeof(data.location)));
         Add(new cMenuEditIntItem(tr("Delay (ms)"), &data.parameter,  0, 0xFFFF));
         break;
    case eProtocolEXT:
         Add(new cMenuEditStrItem(tr("Script"),     data.location, sizeof(data.location)));
         Add(new cMenuEditIntItem(tr("Parameter"), &data.parameter,  0, 0xFFFF));
         break;
    case eProtocolHTTP:
    case eProtocolUDP:
    default:
         Add(new cMenuEditStrItem(tr("Address"), data.location, sizeof(data.location)));
         Add(new cMenuEditIntItem(tr("Port"),   &data.parameter,  0, 0xFFFF));
         break;
    }
  cOsdItem *sidScanItem = new cMenuEditBoolItem(tr("Scan Sid"), &data.sidscan);
  if (!IptvConfig.GetSectionFiltering())
     sidScanItem->SetSelectable(false);
  Add(sidScanItem);
  Add(new cMenuEditBoolItem(tr("Scan pids"),   &data.pidscan));
  // Normal settings
  Add(new cMenuEditStrItem(trVDR("Name"),       data.name,     sizeof(data.name)));
  Add(new cMenuEditIntItem(trVDR("Frequency"), &data.frequency));
  Add(new cMenuEditIntItem(trVDR("Vpid"),      &data.vpid,     0, 0x1FFF));
#if defined(APIVERSNUM) && APIVERSNUM >= 10704
  Add(new cMenuEditIntItem(tr   ("Vtype"),     &data.vtype,    0, 0xFF));
#endif
  Add(new cMenuEditIntItem(trVDR("Ppid"),      &data.ppid,     0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Apid1"),     &data.apid[0],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Apid2"),     &data.apid[1],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Dpid1"),     &data.dpid[0],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Dpid2"),     &data.dpid[1],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Spid1"),     &data.spid[0],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Spid2"),     &data.spid[1],  0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("Tpid"),      &data.tpid,     0, 0x1FFF));
  Add(new cMenuEditIntItem(trVDR("CA"),        &data.caids[0], 0, 0xFFFF));
  Add(new cMenuEditIntItem(trVDR("Sid"),       &data.sid,      1, 0xFFFF));
  Add(new cMenuEditIntItem(tr   ("Nid"),       &data.nid,      0, 0xFFFF));
  Add(new cMenuEditIntItem(tr   ("Tid"),       &data.tid,      0, 0xFFFF));
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
            if (!iteratorChannel->GroupSep() && iteratorChannel != channel &&
                iteratorChannel->GetChannelID() == newchannel.GetChannelID()) {
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
       case eProtocolEXT:
            strn0cpy(data.location, "iptvstream.sh", sizeof(data.location));
            data.parameter = 0;
            break;
       case eProtocolFILE:
            strn0cpy(data.location, "/video/stream.ts", sizeof(data.location));
            data.parameter = 0;
            break;
       case eProtocolHTTP:
            strn0cpy(data.location, "127.0.0.1/TS/1", sizeof(data.location));
            data.parameter = 3000;
            break;
       default:
       case eProtocolUDP:
            strn0cpy(data.location, "127.0.0.1", sizeof(data.location));
            data.parameter = 1234;
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
  SetText(cString::sprintf("%d\t%s", channel->Number(), channel->Name()));
}

// --- cIptvMenuChannels -----------------------------------------------------

class cIptvMenuChannels : public cOsdMenu
{
private:
  void Setup(void);
  cChannel *GetChannel(int Index) const;
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
      if (!channel->GroupSep() && channel->IsPlug() && !strncasecmp(channel->PluginParam(), "IPTV", 4)) {
         cIptvMenuChannelItem *item = new cIptvMenuChannelItem(channel);
         Add(item);
         }
      }
  SetHelp(trVDR("Button$Edit"), trVDR("Button$New"), trVDR("Button$Delete"), NULL);
  Display();
}

cChannel *cIptvMenuChannels::GetChannel(int Index) const
{
  cIptvMenuChannelItem *p = dynamic_cast<cIptvMenuChannelItem *>(Get(Index));
  return p ? (cChannel *)p->Channel() : NULL;
}

void cIptvMenuChannels::Propagate(void)
{
  Channels.ReNumber();
  for (cIptvMenuChannelItem *ci = dynamic_cast<cIptvMenuChannelItem *>(First()); ci; ci = dynamic_cast<cIptvMenuChannelItem *>(ci->Next()))
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

// --- cIptvMenuInfo ---------------------------------------------------------

class cIptvMenuInfo : public cOsdMenu
{
private:
  enum {
    INFO_TIMEOUT_MS = 2000
  };
  cString text;
  cTimeMs timeout;
  unsigned int page;
  void UpdateInfo();

public:
  cIptvMenuInfo();
  virtual ~cIptvMenuInfo();
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys Key);
};

cIptvMenuInfo::cIptvMenuInfo()
:cOsdMenu(tr("IPTV Information")), text(""), timeout(), page(IPTV_DEVICE_INFO_GENERAL)
{
  timeout.Set(INFO_TIMEOUT_MS);
  UpdateInfo();
  SetHelp(tr("General"), tr("Pids"), tr("Filters"), tr("Bits/bytes"));
}

cIptvMenuInfo::~cIptvMenuInfo()
{
}

void cIptvMenuInfo::UpdateInfo()
{
  cIptvDevice *device = cIptvDevice::GetIptvDevice(cDevice::ActualDevice()->CardIndex());
  if (device)
     text = device->GetInformation(page);
  else
     text = cString(tr("IPTV information not available!"));
  Display();
  timeout.Set(INFO_TIMEOUT_MS);
}

void cIptvMenuInfo::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetText(text, true);
  if (*text)
     cStatus::MsgOsdTextItem(text);
}

eOSState cIptvMenuInfo::ProcessKey(eKeys Key)
{
  switch (Key) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft, NORMALKEY(Key) == kLeft || NORMALKEY(Key) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(Key) == kUp || NORMALKEY(Key) == kLeft);
                  return osContinue;
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kOk:     return osBack;
       case kRed:    page = IPTV_DEVICE_INFO_GENERAL;
                     UpdateInfo();
                     break;
       case kGreen:  page = IPTV_DEVICE_INFO_PIDS;
                     UpdateInfo();
                     break;
       case kYellow: page = IPTV_DEVICE_INFO_FILTERS;
                     UpdateInfo();
                     break;
       case kBlue:   IptvConfig.SetUseBytes(IptvConfig.GetUseBytes() ? 0 : 1);
                     UpdateInfo();
                     break;
       default:      if (timeout.TimedOut())
                        UpdateInfo();
                     state = osContinue;
                     break;
       }
     }
  return state;
}

// --- cIptvPluginSetup ------------------------------------------------------

cIptvPluginSetup::cIptvPluginSetup()
{
  debug("cIptvPluginSetup::cIptvPluginSetup()\n");
  tsBufferSize = IptvConfig.GetTsBufferSize();
  tsBufferPrefill = IptvConfig.GetTsBufferPrefillRatio();
  extProtocolBasePort = IptvConfig.GetExtProtocolBasePort();
  sectionFiltering = IptvConfig.GetSectionFiltering();
  numDisabledFilters = IptvConfig.GetDisabledFiltersCount();
  if (numDisabledFilters > SECTION_FILTER_TABLE_SIZE)
     numDisabledFilters = SECTION_FILTER_TABLE_SIZE;
  for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i) {
      disabledFilterIndexes[i] = IptvConfig.GetDisabledFilters(i);
      disabledFilterNames[i] = tr(section_filter_table[i].description);
      }
  Setup();
  SetHelp(trVDR("Channels"), NULL, NULL, trVDR("Button$Info"));
}

void cIptvPluginSetup::Setup(void)
{
  int current = Current();

  Clear();
  help.Clear();

  Add(new cMenuEditIntItem( tr("TS buffer size [MB]"), &tsBufferSize, 1, 4));
  help.Append(tr("Define a ringbuffer size for transport streams in megabytes.\n\nSmaller sizes help memory consumption, but are more prone to buffer overflows."));

  Add(new cMenuEditIntItem( tr("TS buffer prefill ratio [%]"), &tsBufferPrefill, 0, 40));
  help.Append(tr("Define a prefill ratio of the ringbuffer for transport streams before data is transferred to VDR.\n\nThis is useful if streaming media over a slow or unreliable connection."));

  Add(new cMenuEditIntItem( tr("EXT protocol base port"), &extProtocolBasePort, 0, 0xFFF7));
  help.Append(tr("Define a base port used by EXT protocol.\n\nThe port range is defined by the number of IPTV devices. This setting sets the port which is listened for connections from external applications when using the EXT protocol."));

  Add(new cMenuEditBoolItem(tr("Use section filtering"), &sectionFiltering));
  help.Append(tr("Define whether the section filtering shall be used.\n\nSection filtering means that IPTV plugin tries to parse and provide VDR with secondary data about the currently active stream. VDR can then use this data for providing various functionalities such as automatic pid change detection and EPG etc.\nEnabling this feature does not affect streams that do not contain section data."));

  if (sectionFiltering) {
     Add(new cMenuEditIntItem( tr("Disable filters"), &numDisabledFilters, 0, SECTION_FILTER_TABLE_SIZE));
     help.Append(tr("Define number of section filters to be disabled.\n\nCertain section filters might cause some unwanted behaviour to VDR such as time being falsely synchronized. By black-listing the filters here useful section data can be left intact for VDR to process."));

     for (int i = 0; i < numDisabledFilters; ++i) {
         // TRANSLATORS: note the singular!
         Add(new cMenuEditStraItem(tr("Disable filter"), &disabledFilterIndexes[i], SECTION_FILTER_TABLE_SIZE, disabledFilterNames));
         help.Append(tr("Define an ill-behaving filter to be blacklisted."));
         }
     }

  SetCurrent(Get(current));
  Display();
}

eOSState cIptvPluginSetup::EditChannel(void)
{
  debug("cIptvPluginSetup::EditChannel()\n");
  if (HasSubMenu())
     return osContinue;
  return AddSubMenu(new cIptvMenuChannels());
}

eOSState cIptvPluginSetup::ShowInfo(void)
{
  debug("cIptvPluginSetup::ShowInfo()\n");
  if (HasSubMenu())
     return osContinue;
  return AddSubMenu(new cIptvMenuInfo());
}

eOSState cIptvPluginSetup::ProcessKey(eKeys Key)
{
  int oldsectionFiltering = sectionFiltering;
  int oldNumDisabledFilters = numDisabledFilters;
  eOSState state = cMenuSetupPage::ProcessKey(Key);

  if (state == osUnknown) {
     switch (Key) {
       case kRed:  return EditChannel();
       case kBlue: return ShowInfo();
       case kInfo: if (Current() < help.Size())
                      return AddSubMenu(new cMenuText(cString::sprintf("%s - %s '%s'", tr("Help"), trVDR("Plugin"), PLUGIN_NAME_I18N), help[Current()]));
       default:    state = osContinue; break;
       }
     }

  if ((Key != kNone) && ((numDisabledFilters != oldNumDisabledFilters) || (sectionFiltering != oldsectionFiltering))) {
     while ((numDisabledFilters < oldNumDisabledFilters) && (oldNumDisabledFilters > 0))
           disabledFilterIndexes[--oldNumDisabledFilters] = -1;
     Setup();
     }

  return state;
}

void cIptvPluginSetup::StoreFilters(const char *Name, int *Values)
{
  char buffer[SECTION_FILTER_TABLE_SIZE * 4];
  char *q = buffer;
  for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i) {
      char s[3];
      if (Values[i] < 0)
         break;
      if (q > buffer)
         *q++ = ' ';
      snprintf(s, sizeof(s), "%d", Values[i]);
      strncpy(q, s, strlen(s));
      q += strlen(s);
      }
  *q = 0;
  debug("cIptvPluginSetup::StoreFilters(): %s=%s\n", Name, buffer);
  SetupStore(Name, buffer);
}

void cIptvPluginSetup::Store(void)
{
  // Store values into setup.conf
  SetupStore("TsBufferSize", tsBufferSize);
  SetupStore("TsBufferPrefill", tsBufferPrefill);
  SetupStore("ExtProtocolBasePort", extProtocolBasePort);
  SetupStore("SectionFiltering", sectionFiltering);
  StoreFilters("DisabledFilters", disabledFilterIndexes);
  // Update global config
  IptvConfig.SetTsBufferSize(tsBufferSize);
  IptvConfig.SetTsBufferPrefillRatio(tsBufferPrefill);
  IptvConfig.SetExtProtocolBasePort(extProtocolBasePort);
  IptvConfig.SetSectionFiltering(sectionFiltering);
  for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i)
      IptvConfig.SetDisabledFilters(i, disabledFilterIndexes[i]);
}
