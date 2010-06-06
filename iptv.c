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

#if defined(APIVERSNUM) && APIVERSNUM < 10715
#error "VDR-1.7.15 API version or greater is required!"
#endif

static const char VERSION[]     = "0.4.2";
static const char DESCRIPTION[] = trNOOP("Experience the IPTV");

class cPluginIptv : public cPlugin {
private:
  unsigned int deviceCount;
  int ParseFilters(const char *Value, int *Values);
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
: deviceCount(1)
{
  //debug("cPluginIptv::cPluginIptv()\n");
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginIptv::~cPluginIptv()
{
  //debug("cPluginIptv::~cPluginIptv()\n");
  // Clean up after yourself!
}

const char *cPluginIptv::CommandLineHelp(void)
{
  debug("cPluginIptv::CommandLineHelp()\n");
  // Return a string that describes all known command line options.
  return "  -d <num>, --devices=<number> number of devices to be created\n";
}

bool cPluginIptv::ProcessArgs(int argc, char *argv[])
{
  debug("cPluginIptv::ProcessArgs()\n");
  // Implement command line argument processing here if applicable.
  static const struct option long_options[] = {
    { "devices", required_argument, NULL, 'd' },
    { NULL,      no_argument,       NULL,  0  }
    };

  int c;
  while ((c = getopt_long(argc, argv, "d:", long_options, NULL)) != -1) {
    switch (c) {
      case 'd':
           deviceCount = atoi(optarg);
           break;
      default:
           return false;
      }
    }
  return true;
}

bool cPluginIptv::Initialize(void)
{
  debug("cPluginIptv::Initialize()\n");
  // Initialize any background activities the plugin shall perform.
  IptvConfig.SetConfigDirectory(cPlugin::ConfigDirectory(PLUGIN_NAME_I18N));
  return cIptvDevice::Initialize(deviceCount);
}

bool cPluginIptv::Start(void)
{
  debug("cPluginIptv::Start()\n");
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginIptv::Stop(void)
{
  debug("cPluginIptv::Stop()\n");
  // Stop any background activities the plugin is performing.
}

void cPluginIptv::Housekeeping(void)
{
  //debug("cPluginIptv::Housekeeping()\n");
  // Perform any cleanup or other regular tasks.
}

void cPluginIptv::MainThreadHook(void)
{
  //debug("cPluginIptv::MainThreadHook()\n");
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginIptv::Active(void)
{
  //debug("cPluginIptv::Active()\n");
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginIptv::WakeupTime(void)
{
  //debug("cPluginIptv::WakeupTime()\n");
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginIptv::MainMenuAction(void)
{
  //debug("cPluginIptv::MainMenuAction()\n");
  // Perform the action when selected from the main VDR menu.
  return NULL;
}

cMenuSetupPage *cPluginIptv::SetupMenu(void)
{
  debug("cPluginIptv::SetupMenu()\n");
  // Return a setup menu in case the plugin supports one.
  return new cIptvPluginSetup();
}

int cPluginIptv::ParseFilters(const char *Value, int *Filters)
{
  debug("cPluginIptv::ParseFilters(): Value=%s\n", Value);
  char buffer[256];
  int n = 0;
  while (Value && *Value && (n < SECTION_FILTER_TABLE_SIZE)) {
    strn0cpy(buffer, Value, sizeof(buffer));
    int i = atoi(buffer);
    //debug("cPluginIptv::ParseFilters(): Filters[%d]=%d\n", n, i);
    if (i >= 0)
       Filters[n++] = i;
    if ((Value = strchr(Value, ' ')) != NULL)
       Value++;
    }
  return n;
}

bool cPluginIptv::SetupParse(const char *Name, const char *Value)
{
  debug("cPluginIptv::SetupParse()\n");
  // Parse your own setup parameters and store their values.
  if (!strcasecmp(Name, "TsBufferSize"))
     IptvConfig.SetTsBufferSize(atoi(Value));
  else if (!strcasecmp(Name, "TsBufferPrefill"))
     IptvConfig.SetTsBufferPrefillRatio(atoi(Value));
  else if (!strcasecmp(Name, "ExtProtocolBasePort"))
     IptvConfig.SetExtProtocolBasePort(atoi(Value));
  else if (!strcasecmp(Name, "SectionFiltering"))
     IptvConfig.SetSectionFiltering(atoi(Value));
  else if (!strcasecmp(Name, "DisabledFilters")) {
     int DisabledFilters[SECTION_FILTER_TABLE_SIZE];
     for (unsigned int i = 0; i < ARRAY_SIZE(DisabledFilters); ++i)
         DisabledFilters[i] = -1;
     unsigned int DisabledFiltersCount = ParseFilters(Value, DisabledFilters);
     for (unsigned int i = 0; i < DisabledFiltersCount; ++i)
         IptvConfig.SetDisabledFilters(i, DisabledFilters[i]);
     }
  else
     return false;
  return true;
}

bool cPluginIptv::Service(const char *Id, void *Data)
{
  //debug("cPluginIptv::Service()\n");
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginIptv::SVDRPHelpPages(void)
{
  debug("cPluginIptv::SVDRPHelpPages()\n");
  static const char *HelpPages[] = {
    "INFO [ <page> ]\n"
    "    Print IPTV device information and statistics.\n"
    "    The output can be narrowed using optional \"page\""
    "    option: 1=general 2=pids 3=section filters.\n",
    "MODE\n"
    "    Toggles between bit or byte information mode.\n",
    NULL
    };
  return HelpPages;
}

cString cPluginIptv::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  debug("cPluginIptv::SVDRPCommand(): Command=%s Option=%s\n", Command, Option);
  if (strcasecmp(Command, "INFO") == 0) {
     cIptvDevice *device = cIptvDevice::GetIptvDevice(cDevice::ActualDevice()->CardIndex());
     if (device) {
        int page = IPTV_DEVICE_INFO_ALL;
        if (Option) {
           page = atoi(Option);
           if ((page < IPTV_DEVICE_INFO_ALL) || (page > IPTV_DEVICE_INFO_FILTERS))
              page = IPTV_DEVICE_INFO_ALL;
           }
        return device->GetInformation(page);
        }
     else {
        ReplyCode = 550; // Requested action not taken
        return cString("IPTV information not available!");
        }
     }
  else if (strcasecmp(Command, "MODE") == 0) {
     unsigned int mode = !IptvConfig.GetUseBytes();
     IptvConfig.SetUseBytes(mode);
     return cString::sprintf("IPTV information mode is: %s\n", mode ? "bytes" : "bits");
     }
  return NULL;
}

VDRPLUGINCREATOR(cPluginIptv); // Don't touch this!
