/*
 * protocolfile.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>

#include "common.h"
#include "config.h"
#include "protocolfile.h"

cIptvProtocolFile::cIptvProtocolFile()
: fileLocationM(strdup("")),
  fileDelayM(0),
  fileStreamM(NULL),
  isActiveM(false)
{
  debug("cIptvProtocolFile::%s()", __FUNCTION__);
}

cIptvProtocolFile::~cIptvProtocolFile()
{
  debug("cIptvProtocolFile::%s()", __FUNCTION__);
  // Drop open handles
  cIptvProtocolFile::Close();
  // Free allocated memory
  free(fileLocationM);
}

bool cIptvProtocolFile::OpenFile(void)
{
  debug("cIptvProtocolFile::%s()", __FUNCTION__);
  // Check that stream address is valid
  if (!isActiveM && !isempty(fileLocationM)) {
     fileStreamM = fopen(fileLocationM, "rb");
     ERROR_IF_RET(!fileStreamM || ferror(fileStreamM), "fopen()", return false);
     // Update active flag
     isActiveM = true;
     }
  return true;
}

void cIptvProtocolFile::CloseFile(void)
{
  debug("cIptvProtocolFile::%s()", __FUNCTION__);
  // Check that file stream is valid
  if (isActiveM && !isempty(fileLocationM)) {
     fclose(fileStreamM);
     // Update active flag
     isActiveM = false;
     }
}

int cIptvProtocolFile::Read(unsigned char* bufferAddrP, unsigned int bufferLenP)
{
   //debug("cIptvProtocolFile::%s()", __FUNCTION__);
   // Check errors
   if (ferror(fileStreamM)) {
      debug("cIptvProtocolFile::%s(): stream error", __FUNCTION__);
      return -1;
      }
   // Rewind if EOF
   if (feof(fileStreamM))
      rewind(fileStreamM);
   // Sleep before reading the file stream to prevent aggressive busy looping
   // and prevent transfer ringbuffer overflows
   if (fileDelayM)
      cCondWait::SleepMs(fileDelayM);
   // This check is to prevent a race condition where file may be switched off
   // during the sleep and buffers are disposed. Check here that the plugin is
   // still active before accessing the buffers
   if (isActiveM)
      return (int)fread(bufferAddrP, sizeof(unsigned char), bufferLenP, fileStreamM);
   return -1;
}

bool cIptvProtocolFile::Open(void)
{
  debug("cIptvProtocolFile::%s()", __FUNCTION__);
  // Open the file stream
  OpenFile();
  return true;
}

bool cIptvProtocolFile::Close(void)
{
  debug("cIptvProtocolFile::%s()", __FUNCTION__);
  // Close the file stream
  CloseFile();
  return true;
}

bool cIptvProtocolFile::Set(const char* locationP, const int parameterP, const int indexP)
{
  debug("cIptvProtocolFile::%s(%s, %d, %d)", __FUNCTION__, locationP, parameterP, indexP);
  if (!isempty(locationP)) {
     // Close the file stream
     CloseFile();
     // Update stream address and port
     fileLocationM = strcpyrealloc(fileLocationM, locationP);
     fileDelayM = parameterP;
     // Open the file for input
     OpenFile();
     }
  return true;
}

cString cIptvProtocolFile::GetInformation(void)
{
  //debug("cIptvProtocolFile::%s()", __FUNCTION__);
  return cString::sprintf("file://%s:%d", fileLocationM, fileDelayM);
}
