/*
 * source.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <ctype.h>

#include "common.h"
#include "log.h"
#include "source.h"

// --- cIptvTransponderParameters --------------------------------------------

cIptvTransponderParameters::cIptvTransponderParameters(const char *parametersP)
: sidScanM(0),
  pidScanM(0),
  protocolM(eProtocolUDP),
  parameterM(0)
{
  debug1("%s (%s)", __PRETTY_FUNCTION__, parametersP);

  memset(&addressM, 0, sizeof(addressM));
  Parse(parametersP);
}

cString cIptvTransponderParameters::ToString(char typeP) const
{
  debug1("%s (%c)", __PRETTY_FUNCTION__, typeP);

  const char *protocolstr;

  switch (protocolM) {
    case eProtocolEXT:
         protocolstr = "EXT";
         break;
    case eProtocolCURL:
         protocolstr = "CURL";
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
  return cString::sprintf("S=%d|P=%d|F=%s|U=%s|A=%d", sidScanM, pidScanM, protocolstr, addressM, parameterM);
}

bool cIptvTransponderParameters::Parse(const char *strP)
{
  debug1("%s (%s)", __PRETTY_FUNCTION__, strP);
  bool result = false;

  if (strP && *strP) {
     const char *delim = "|";
     char *str = strdup(strP);
     char *p = str;
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
          switch (toupper(*token)) {
            case 'S':
                 sidScanM = (int)strtol(data, (char **)NULL, 10);
                 found_s = true;
                 break;
            case 'P':
                 pidScanM = (int)strtol(data, (char **)NULL, 10);
                 found_p = true;
                 break;
            case 'F':
                 if (strstr(data, "UDP")) {
                    protocolM = eProtocolUDP;
                    found_f = true;
                    }
                 else if (strstr(data, "CURL")) {
                    protocolM = eProtocolCURL;
                    found_f = true;
                    }
                 else if (strstr(data, "HTTP")) {
                    protocolM = eProtocolHTTP;
                    found_f = true;
                    }
                 else if (strstr(data, "FILE")) {
                    protocolM = eProtocolFILE;
                    found_f = true;
                    }
                 else if (strstr(data, "EXT")) {
                    protocolM = eProtocolEXT;
                    found_f = true;
                    }
                 break;
            case 'U':
                 strn0cpy(addressM, data, sizeof(addressM));
                 found_u = true;
                 break;
            case 'A':
                 parameterM = (int)strtol(data, (char **)NULL, 10);
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
        error("Invalid channel parameters: %s", p);

     free(p);
     }

  return (result);
}

// --- cIptvSourceParam ------------------------------------------------------

const char *cIptvSourceParam::allowedProtocolCharsS = " abcdefghijklmnopqrstuvwxyz0123456789-.,#~\\^$[]()*+?{}/%@&=";

cIptvSourceParam::cIptvSourceParam(char sourceP, const char *descriptionP)
  : cSourceParam(sourceP, descriptionP),
    paramM(0),
    ridM(0),
    dataM(),
    itpM()
{
  debug1("%s (%c, %s)", __PRETTY_FUNCTION__, sourceP, descriptionP);

  protocolsM[cIptvTransponderParameters::eProtocolUDP]  = tr("UDP");
  protocolsM[cIptvTransponderParameters::eProtocolCURL] = tr("CURL");
  protocolsM[cIptvTransponderParameters::eProtocolHTTP] = tr("HTTP");
  protocolsM[cIptvTransponderParameters::eProtocolFILE] = tr("FILE");
  protocolsM[cIptvTransponderParameters::eProtocolEXT]  = tr("EXT");
}

void cIptvSourceParam::SetData(cChannel *channelP)
{
  debug1("%s (%s)", __PRETTY_FUNCTION__, channelP->Parameters());
  dataM = *channelP;
  ridM = dataM.Rid();
  itpM.Parse(dataM.Parameters());
  paramM = 0;
}

void cIptvSourceParam::GetData(cChannel *channelP)
{
  debug1("%s (%s)", __PRETTY_FUNCTION__, channelP->Parameters());
  channelP->SetTransponderData(channelP->Source(), channelP->Frequency(), dataM.Srate(), itpM.ToString(Source()), true);
  channelP->SetId(channelP->Nid(), channelP->Tid(), channelP->Sid(), ridM);
}

cOsdItem *cIptvSourceParam::GetOsdItem(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  switch (paramM++) {
    case  0: return new cMenuEditIntItem( tr("Rid"),              &ridM, 0);
    case  1: return new cMenuEditBoolItem(tr("Scan section ids"), &itpM.sidScanM);
    case  2: return new cMenuEditBoolItem(tr("Scan pids"),        &itpM.pidScanM);
    case  3: return new cMenuEditStraItem(tr("Protocol"),         &itpM.protocolM,  ELEMENTS(protocolsM),  protocolsM);
    case  4: return new cMenuEditStrItem( tr("Address"),           itpM.addressM,   sizeof(itpM.addressM), allowedProtocolCharsS);
    case  5: return new cMenuEditIntItem( tr("Parameter"),        &itpM.parameterM, 0,                     0xFFFF);
    default: return NULL;
    }
  return NULL;
}
