/*
 * setup.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <vdr/status.h>
#include <vdr/menu.h>

#include "common.h"
#include "config.h"
#include "device.h"
#include "setup.h"

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
  SetHelp(NULL, NULL, NULL, trVDR("Button$Info"));
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
