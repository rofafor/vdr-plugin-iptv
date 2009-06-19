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
: fileDelay(0),
  fileStream(NULL),
  isActive(false)
{
  debug("cIptvProtocolFile::cIptvProtocolFile()\n");
  fileLocation = strdup("");
}

cIptvProtocolFile::~cIptvProtocolFile()
{
  debug("cIptvProtocolFile::~cIptvProtocolFile()\n");
  // Drop open handles
  cIptvProtocolFile::Close();
  // Free allocated memory
  free(fileLocation);
}

bool cIptvProtocolFile::OpenFile(void)
{
  debug("cIptvProtocolFile::OpenFile()\n");
  // Check that stream address is valid
  if (!isActive && !isempty(fileLocation)) {
     fileStream = fopen(fileLocation, "rb");
     ERROR_IF_RET(!fileStream || ferror(fileStream), "fopen()", return false);
     // Update active flag
     isActive = true;
     }
  return true;
}

void cIptvProtocolFile::CloseFile(void)
{
  debug("cIptvProtocolFile::CloseFile()\n");
  // Check that file stream is valid
  if (isActive && !isempty(fileLocation)) {
     fclose(fileStream);
     // Update active flag
     isActive = false;
     }
}

int cIptvProtocolFile::Read(unsigned char* BufferAddr, unsigned int BufferLen)
{
   //debug("cIptvProtocolFile::Read()\n");
   // Check errors
   if (ferror(fileStream)) {
      debug("Read error\n");
      return -1;
      }
   // Rewind if EOF
   if (feof(fileStream))
      rewind(fileStream);
   // Sleep before reading the file stream to prevent aggressive busy looping
   // and prevent transfer ringbuffer overflows
   if (fileDelay)
      cCondWait::SleepMs(fileDelay);
   // This check is to prevent a race condition where file may be switched off
   // during the sleep and buffers are disposed. Check here that the plugin is
   // still active before accessing the buffers
   if (isActive)
      return (int)fread(BufferAddr, sizeof(unsigned char), BufferLen, fileStream);
   return -1;
}

bool cIptvProtocolFile::Open(void)
{
  debug("cIptvProtocolFile::Open()\n");
  // Open the file stream
  OpenFile();
  return true;
}

bool cIptvProtocolFile::Close(void)
{
  debug("cIptvProtocolFile::Close()\n");
  // Close the file stream
  CloseFile();
  return true;
}

bool cIptvProtocolFile::Set(const char* Location, const int Parameter, const int Index)
{
  debug("cIptvProtocolFile::Set(): Location=%s Parameter=%d Index=%d\n", Location, Parameter, Index);
  if (!isempty(Location)) {
     // Close the file stream
     CloseFile();
     // Update stream address and port
     fileLocation = strcpyrealloc(fileLocation, Location);
     fileDelay = Parameter;
     // Open the file for input
     OpenFile();
     }
  return true;
}

cString cIptvProtocolFile::GetInformation(void)
{
  //debug("cIptvProtocolFile::GetInformation()");
  return cString::sprintf("file://%s:%d", fileLocation, fileDelay);
}
