/*
 * iptv.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <getopt.h>
#include <vdr/plugin.h>
#include "common.h"
#include "config.h"
#include "setup.h"
#include "device.h"
#include "iptvservice.h"

#if defined(APIVERSNUM) && APIVERSNUM < 20200
#error "VDR-2.2.0 API version or greater is required!"
#endif

#ifndef GITVERSION
#define GITVERSION ""
#endif

       const char VERSION[]     = "2.2.1" GITVERSION;
static const char DESCRIPTION[] = trNOOP("Experience the IPTV");

class cPluginIptv : public cPlugin {
private:
  unsigned int deviceCountM;
  int ParseFilters(const char *valueP, int *filtersP);
public:
  cPluginIptv(void);
  virtual ~cPluginIptv();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return NULL; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginIptv::cPluginIptv(void)
: deviceCountM(1)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginIptv::~cPluginIptv()
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Clean up after yourself!
}

const char *cPluginIptv::CommandLineHelp(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Return a string that describes all known command line options.
  return "  -d <num>, --devices=<number>  number of devices to be created\n"
         "  -t <mode>, --trace=<mode>     set the tracing mode\n";
}

bool cPluginIptv::ProcessArgs(int argc, char *argv[])
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Implement command line argument processing here if applicable.
  static const struct option long_options[] = {
    { "devices", required_argument, NULL, 'd' },
    { "trace",    required_argument, NULL, 't' },
    { NULL,      no_argument,       NULL,  0  }
    };

  int c;
  while ((c = getopt_long(argc, argv, "d:", long_options, NULL)) != -1) {
    switch (c) {
      case 'd':
           deviceCountM = atoi(optarg);
           break;
      case 't':
           IptvConfig.SetTraceMode(strtol(optarg, NULL, 0));
           break;
      default:
           return false;
      }
    }
  return true;
}

bool cPluginIptv::Initialize(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Initialize any background activities the plugin shall perform.
  IptvConfig.SetConfigDirectory(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N));
  IptvConfig.SetResourceDirectory(cPlugin::ResourceDirectory(PLUGIN_NAME_I18N));
  return cIptvDevice::Initialize(deviceCountM);
}

bool cPluginIptv::Start(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Start any background activities the plugin shall perform.
  if (curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK) {
     curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);
     cString info = cString::sprintf("Using CURL %s", data->version);
     for (int i = 0; data->protocols[i]; ++i) {
         // Supported protocols: HTTP(S), RTSP, FILE
         if (startswith(data->protocols[i], "http") || startswith(data->protocols[i], "rtsp") ||
             startswith(data->protocols[i], "file"))
            info = cString::sprintf("%s %s", *info, data->protocols[i]);
         }
     info("%s", *info);
     }
  return true;
}

void cPluginIptv::Stop(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Stop any background activities the plugin is performing.
  cIptvDevice::Shutdown();
  curl_global_cleanup();
}

void cPluginIptv::Housekeeping(void)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Perform any cleanup or other regular tasks.
}

void cPluginIptv::MainThreadHook(void)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginIptv::Active(void)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginIptv::WakeupTime(void)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginIptv::MainMenuAction(void)
{
  debug16("%s", __PRETTY_FUNCTION__);
  // Perform the action when selected from the main VDR menu.
  return NULL;
}

cMenuSetupPage *cPluginIptv::SetupMenu(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Return a setup menu in case the plugin supports one.
  return new cIptvPluginSetup();
}

int cPluginIptv::ParseFilters(const char *valueP, int *filtersP)
{
  debug1("%s (%s, )", __PRETTY_FUNCTION__, valueP);
  char buffer[256];
  int n = 0;
  while (valueP && *valueP && (n < SECTION_FILTER_TABLE_SIZE)) {
    strn0cpy(buffer, valueP, sizeof(buffer));
    int i = atoi(buffer);
    debug16("%s (%s, ) filters[%d]=%d", __PRETTY_FUNCTION__, valueP, n, i);
    if (i >= 0)
       filtersP[n++] = i;
    if ((valueP = strchr(valueP, ' ')) != NULL)
       valueP++;
    }
  return n;
}

bool cPluginIptv::SetupParse(const char *nameP, const char *valueP)
{
  debug1("%s (%s, %s)", __PRETTY_FUNCTION__, nameP, valueP);
  // Parse your own setup parameters and store their values.
  if (!strcasecmp(nameP, "ExtProtocolBasePort"))
     IptvConfig.SetProtocolBasePort(atoi(valueP));
  else if (!strcasecmp(nameP, "SectionFiltering"))
     IptvConfig.SetSectionFiltering(atoi(valueP));
  else if (!strcasecmp(nameP, "DisabledFilters")) {
     int DisabledFilters[SECTION_FILTER_TABLE_SIZE];
     for (unsigned int i = 0; i < ARRAY_SIZE(DisabledFilters); ++i)
         DisabledFilters[i] = -1;
     unsigned int DisabledFiltersCount = ParseFilters(valueP, DisabledFilters);
     for (unsigned int i = 0; i < DisabledFiltersCount; ++i)
         IptvConfig.SetDisabledFilters(i, DisabledFilters[i]);
     }
  else
     return false;
  return true;
}

bool cPluginIptv::Service(const char *idP, void *dataP)
{
  debug1("%s (%s, )", __PRETTY_FUNCTION__, idP);
  if (strcmp(idP, "IptvService-v1.0") == 0) {
     if (dataP) {
        IptvService_v1_0 *data = reinterpret_cast<IptvService_v1_0*>(dataP);
        cIptvDevice *dev = cIptvDevice::GetIptvDevice(data->cardIndex);
        if (!dev)
           return false;
        data->protocol = dev->GetInformation(IPTV_DEVICE_INFO_PROTOCOL);
        data->bitrate  = dev->GetInformation(IPTV_DEVICE_INFO_BITRATE);
        }
     return true;
     }
  return false;
}

const char **cPluginIptv::SVDRPHelpPages(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  static const char *HelpPages[] = {
    "INFO [ <page> ]\n"
    "    Print IPTV device information and statistics.\n"
    "    The output can be narrowed using optional \"page\""
    "    option: 1=general 2=pids 3=section filters.\n",
    "MODE\n"
    "    Toggles between bit or byte information mode.\n",
    "TRAC [ <mode> ]\n"
    "    Gets and/or sets used tracing mode.\n",
    NULL
    };
  return HelpPages;
}

cString cPluginIptv::SVDRPCommand(const char *commandP, const char *optionP, int &replyCodeP)
{
  debug1("%s (%s, %s, )", __PRETTY_FUNCTION__, commandP, optionP);
  if (strcasecmp(commandP, "INFO") == 0) {
     cIptvDevice *device = cIptvDevice::GetIptvDevice(cDevice::ActualDevice()->CardIndex());
     if (device) {
        int page = IPTV_DEVICE_INFO_ALL;
        if (optionP) {
           page = atoi(optionP);
           if ((page < IPTV_DEVICE_INFO_ALL) || (page > IPTV_DEVICE_INFO_FILTERS))
              page = IPTV_DEVICE_INFO_ALL;
           }
        return device->GetInformation(page);
        }
     else {
        replyCodeP = 550; // Requested action not taken
        return cString("IPTV information not available!");
        }
     }
  else if (strcasecmp(commandP, "MODE") == 0) {
     unsigned int mode = !IptvConfig.GetUseBytes();
     IptvConfig.SetUseBytes(mode);
     return cString::sprintf("IPTV information mode is: %s\n", mode ? "bytes" : "bits");
     }
  else if (strcasecmp(commandP, "TRAC") == 0) {
     if (optionP && *optionP)
        IptvConfig.SetTraceMode(strtol(optionP, NULL, 0));
     return cString::sprintf("IPTV tracing mode: 0x%04X\n", IptvConfig.GetTraceMode());
     }

  return NULL;
}

VDRPLUGINCREATOR(cPluginIptv); // Don't touch this!
