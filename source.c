/*
 * source.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include "common.h"
#include "source.h"

// --- cIptvTransponderParameters --------------------------------------------

cIptvTransponderParameters::cIptvTransponderParameters(const char *Parameters)
: sidscan(0),
  pidscan(0),
  protocol(eProtocolUDP),
  parameter(0)
{
  debug("cIptvTransponderParameters::cIptvTransponderParameters(): Parameters=%s\n", Parameters);

  memset(&address, 0, sizeof(address));
  Parse(Parameters);
}

cString cIptvTransponderParameters::ToString(char Type) const
{
  debug("cIptvTransponderParameters::ToString() Type=%c\n", Type);

  const char *protocolstr;

  switch (protocol) {
    case eProtocolEXT:
         protocolstr = "EXT";
         break;
    case eProtocolHTTP:
         protocolstr = "HTTP";
         break;
    case eProtocolFILE:
         protocolstr = "FILE";
         break;
    default:
    case eProtocolUDP:
         protocolstr = "UDP";
         break;
  }
  return cString::sprintf("S=%d|P=%d|F=%s|U=%s|A=%d", sidscan, pidscan, protocolstr, address, parameter);
}

bool cIptvTransponderParameters::Parse(const char *s)
{
  debug("cIptvTransponderParameters::Parse(): s=%s\n", s);
  bool result = false;

  if (s && *s) {
     const char *delim = "|";
     char *str = strdup(s);
     char *saveptr = NULL;
     char *token = NULL;
     bool found_s = false;
     bool found_p = false;
     bool found_f = false;
     bool found_u = false;
     bool found_a = false;

     while ((token = strtok_r(str, delim, &saveptr)) != NULL) {
       char *data = token;
       ++data;
       if (data && (*data == '=')) {
          ++data;
          switch (*token) {
            case 'S':
                 sidscan = (int)strtol(data, (char **)NULL, 10);
                 found_s = true;
                 break;
            case 'P':
                 pidscan = (int)strtol(data, (char **)NULL, 10);
                 found_p = true;
                 break;
            case 'F':
                 if (strstr(data, "UDP")) {
                    protocol = eProtocolUDP;
                    found_f = true;
                    }
                 else if (strstr(data, "HTTP")) {
                    protocol = eProtocolHTTP;
                    found_f = true;
                    }
                 else if (strstr(data, "FILE")) {
                    protocol = eProtocolFILE;
                    found_f = true;
                    }
                 else if (strstr(data, "EXT")) {
                    protocol = eProtocolEXT;
                    found_f = true;
                    }
                 break;
            case 'U':
                 strn0cpy(address, data, sizeof(address));
                 found_u = true;
                 break;
            case 'A':
                 parameter = (int)strtol(data, (char **)NULL, 10);
                 found_a = true;
                 break;
            default:
                 break;
            }
          }
       str = NULL;
       }

     if (found_s && found_p && found_f && found_u && found_a)
        result = true;
     else
        error("Invalid channel parameters: %s\n", str);

     free(str);
     }

  return (result);
}

// --- cIptvSourceParam ------------------------------------------------------

cIptvSourceParam::cIptvSourceParam(char Source, const char *Description)
  : cSourceParam(Source, Description),
    param(0),
    nid(0),
    tid(0),
    rid(0)
{
  debug("cIptvSourceParam::cIptvSourceParam(): Source=%c Description=%s\n", Source, Description);

  protocols[cIptvTransponderParameters::eProtocolUDP]  = tr("UDP");
  protocols[cIptvTransponderParameters::eProtocolHTTP] = tr("HTTP");
  protocols[cIptvTransponderParameters::eProtocolFILE] = tr("FILE");
  protocols[cIptvTransponderParameters::eProtocolEXT]  = tr("EXT");
}

void cIptvSourceParam::SetData(cChannel *Channel)
{
  debug("cIptvSourceParam::SetData(): Channel=%s)\n", Channel->Parameters());
  data = *Channel;
  nid = data.Nid();
  tid = data.Tid();
  rid = data.Rid();
  itp.Parse(data.Parameters());
  param = 0;
}

void cIptvSourceParam::GetData(cChannel *Channel)
{
  debug("cIptvSourceParam::GetData(): Channel=%s\n", Channel->Parameters());
  data.SetTransponderData(Channel->Source(), Channel->Frequency(), data.Srate(), itp.ToString(Source()), true);
  data.SetId(nid, tid, Channel->Sid(), rid);
  *Channel = data;
}

cOsdItem *cIptvSourceParam::GetOsdItem(void)
{
  debug("cIptvSourceParam::GetOsdItem()\n");
  switch (param++) {
    case  0: return new cMenuEditIntItem( tr("Nid"),       &nid,      0);
    case  1: return new cMenuEditIntItem( tr("Tid"),       &tid,      0);
    case  2: return new cMenuEditIntItem( tr("Rid"),       &rid,      0);
    case  3: return new cMenuEditBoolItem(tr("Scan sid"),  &itp.sidscan);
    case  4: return new cMenuEditBoolItem(tr("Scan pids"), &itp.pidscan);
    case  5: return new cMenuEditStraItem(tr("Protocol"),  &itp.protocol,  ELEMENTS(protocols), protocols);
    case  6: return new cMenuEditStrItem( tr("Address"),    itp.address,   sizeof(itp.address));
    case  7: return new cMenuEditIntItem( tr("Parameter"), &itp.parameter, 0,                   0xFFFF);
    default: return NULL;
    }
  return NULL;
}
