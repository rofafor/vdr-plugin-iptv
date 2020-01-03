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
  cString textM;
  cTimeMs timeoutM;
  unsigned int pageM;
  void UpdateInfo();

public:
  cIptvMenuInfo();
  virtual ~cIptvMenuInfo();
  virtual void Display(void);
  virtual eOSState ProcessKey(eKeys keyP);
};

cIptvMenuInfo::cIptvMenuInfo()
:cOsdMenu(tr("IPTV Information")), textM(""), timeoutM(), pageM(IPTV_DEVICE_INFO_GENERAL)
{
  SetMenuCategory(mcText);
  timeoutM.Set(INFO_TIMEOUT_MS);
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
     textM = device->GetInformation(pageM);
  else
     textM = cString(tr("IPTV information not available!"));
  Display();
  timeoutM.Set(INFO_TIMEOUT_MS);
}

void cIptvMenuInfo::Display(void)
{
  cOsdMenu::Display();
  DisplayMenu()->SetText(textM, true);
  if (*textM)
     cStatus::MsgOsdTextItem(textM);
}

eOSState cIptvMenuInfo::ProcessKey(eKeys keyP)
{
  switch (int(keyP)) {
    case kUp|k_Repeat:
    case kUp:
    case kDown|k_Repeat:
    case kDown:
    case kLeft|k_Repeat:
    case kLeft:
    case kRight|k_Repeat:
    case kRight:
                  DisplayMenu()->Scroll(NORMALKEY(keyP) == kUp || NORMALKEY(keyP) == kLeft, NORMALKEY(keyP) == kLeft || NORMALKEY(keyP) == kRight);
                  cStatus::MsgOsdTextItem(NULL, NORMALKEY(keyP) == kUp || NORMALKEY(keyP) == kLeft);
                  return osContinue;
    default: break;
    }

  eOSState state = cOsdMenu::ProcessKey(keyP);

  if (state == osUnknown) {
     switch (keyP) {
       case kOk:     return osBack;
       case kRed:    pageM = IPTV_DEVICE_INFO_GENERAL;
                     UpdateInfo();
                     break;
       case kGreen:  pageM = IPTV_DEVICE_INFO_PIDS;
                     UpdateInfo();
                     break;
       case kYellow: pageM = IPTV_DEVICE_INFO_FILTERS;
                     UpdateInfo();
                     break;
       case kBlue:   IptvConfig.SetUseBytes(IptvConfig.GetUseBytes() ? 0 : 1);
                     UpdateInfo();
                     break;
       default:      if (timeoutM.TimedOut())
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
  debug1("%s", __PRETTY_FUNCTION__);
  protocolBasePortM = IptvConfig.GetProtocolBasePort();
  sectionFilteringM = IptvConfig.GetSectionFiltering();
  numDisabledFiltersM = IptvConfig.GetDisabledFiltersCount();
  if (numDisabledFiltersM > SECTION_FILTER_TABLE_SIZE)
     numDisabledFiltersM = SECTION_FILTER_TABLE_SIZE;
  for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i) {
      disabledFilterIndexesM[i] = IptvConfig.GetDisabledFilters(i);
      disabledFilterNamesM[i] = tr(section_filter_table[i].description);
      }
  SetMenuCategory(mcSetupPlugins);
  Setup();
  SetHelp(NULL, NULL, NULL, trVDR("Button$Info"));
}

void cIptvPluginSetup::Setup(void)
{
  int current = Current();

  Clear();
  helpM.Clear();

  Add(new cMenuEditIntItem(tr("Protocol base port"), &protocolBasePortM, 0, 0xFFFF - MAXDEVICES * 2));
  helpM.Append(tr("Define a base port used by CURL/EXT protocol.\n\nThe port range is defined by the number of IPTV devices. This setting sets the port which is listened for connections from external applications when using the CURL/EXT protocol."));

  Add(new cMenuEditBoolItem(tr("Use section filtering"), &sectionFilteringM));
  helpM.Append(tr("Define whether the section filtering shall be used.\n\nSection filtering means that IPTV plugin tries to parse and provide VDR with secondary data about the currently active stream. VDR can then use this data for providing various functionalities such as automatic pid change detection and EPG etc.\nEnabling this feature does not affect streams that do not contain section data."));

  if (sectionFilteringM) {
     Add(new cMenuEditIntItem(tr("Disabled filters"), &numDisabledFiltersM, 0, SECTION_FILTER_TABLE_SIZE));
     helpM.Append(tr("Define number of section filters to be disabled.\n\nCertain section filters might cause some unwanted behaviour to VDR such as time being falsely synchronized. By black-listing the filters here useful section data can be left intact for VDR to process."));

     for (int i = 0; i < numDisabledFiltersM; ++i) {
         Add(new cMenuEditStraItem(*cString::sprintf(" %s %d", tr("Filter"), i + 1), &disabledFilterIndexesM[i], SECTION_FILTER_TABLE_SIZE, disabledFilterNamesM));
         helpM.Append(tr("Define an ill-behaving filter to be blacklisted."));
         }
     }

  SetCurrent(Get(current));
  Display();
}

eOSState cIptvPluginSetup::ShowInfo(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  if (HasSubMenu())
     return osContinue;
  return AddSubMenu(new cIptvMenuInfo());
}

eOSState cIptvPluginSetup::ProcessKey(eKeys keyP)
{
  int oldsectionFiltering = sectionFilteringM;
  int oldNumDisabledFilters = numDisabledFiltersM;
  eOSState state = cMenuSetupPage::ProcessKey(keyP);

  if (state == osUnknown) {
     switch (keyP) {
       case kBlue: return ShowInfo();
       case kInfo: if (Current() < helpM.Size())
                      return AddSubMenu(new cMenuText(cString::sprintf("%s - %s '%s'", tr("Help"), trVDR("Plugin"), PLUGIN_NAME_I18N), helpM[Current()]));
       default:    state = osContinue; break;
       }
     }

  if ((keyP != kNone) && ((numDisabledFiltersM != oldNumDisabledFilters) || (sectionFilteringM != oldsectionFiltering))) {
     while ((numDisabledFiltersM < oldNumDisabledFilters) && (oldNumDisabledFilters > 0))
           disabledFilterIndexesM[--oldNumDisabledFilters] = -1;
     Setup();
     }

  return state;
}

void cIptvPluginSetup::StoreFilters(const char *nameP, int *valuesP)
{
  char buffer[SECTION_FILTER_TABLE_SIZE * 4];
  char *q = buffer;
  for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i) {
      char s[3];
      if ((valuesP[i] < 0) || (valuesP[i] >= SECTION_FILTER_TABLE_SIZE))
         break;
      if (q > buffer)
         *q++ = ' ';
      snprintf(s, sizeof(s), "%d", valuesP[i]);
      strncpy(q, s, strlen(s));
      q += strlen(s);
      }
  *q = 0;
  debug1("%s (%s, %s)", __PRETTY_FUNCTION__, nameP, buffer);
  SetupStore(nameP, buffer);
}

void cIptvPluginSetup::Store(void)
{
  // Store values into setup.conf
  SetupStore("ExtProtocolBasePort", protocolBasePortM);
  SetupStore("SectionFiltering", sectionFilteringM);
  StoreFilters("DisabledFilters", disabledFilterIndexesM);
  // Update global config
  IptvConfig.SetProtocolBasePort(protocolBasePortM);
  IptvConfig.SetSectionFiltering(sectionFilteringM);
  for (int i = 0; i < SECTION_FILTER_TABLE_SIZE; ++i)
      IptvConfig.SetDisabledFilters(i, disabledFilterIndexesM[i]);
}
