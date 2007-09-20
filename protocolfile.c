/*
 * protocolfile.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolfile.c,v 1.7 2007/09/20 21:45:51 rahrenbe Exp $
 */

#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>

#include "common.h"
#include "config.h"
#include "protocolfile.h"

cIptvProtocolFile::cIptvProtocolFile()
: fileActive(false)
{
  debug("cIptvProtocolFile::cIptvProtocolFile(): %d/%d packets\n",
        IptvConfig.GetFileBufferSize(), IptvConfig.GetMaxBufferSize());
  streamAddr = strdup("");
  // Allocate receive buffer
  readBuffer = MALLOC(unsigned char, (TS_SIZE * IptvConfig.GetMaxBufferSize()));
  if (!readBuffer)
     error("ERROR: MALLOC() failed in ProtocolFile()");
}

cIptvProtocolFile::~cIptvProtocolFile()
{
  debug("cIptvProtocolFile::~cIptvProtocolFile()\n");
  // Drop open handles
  Close();
  // Free allocated memory
  free(streamAddr);
  free(readBuffer);
}

bool cIptvProtocolFile::OpenFile(void)
{
  debug("cIptvProtocolFile::OpenFile()\n");
  // Check that stream address is valid
  if (!fileActive && !isempty(streamAddr)) {
     fileStream = fopen(streamAddr, "rb");
     if (ferror(fileStream) || !fileStream) {
        char tmp[64];
        error("ERROR: fopen(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return false;
        }
     // Update active flag
     fileActive = true;
     }
  return true;
}

void cIptvProtocolFile::CloseFile(void)
{
  debug("cIptvProtocolFile::CloseFile()\n");
  // Check that file stream is valid
  if (fileActive && !isempty(streamAddr)) {
     fclose(fileStream);
     // Update active flag
     fileActive = false;
     }
}

int cIptvProtocolFile::Read(unsigned char* *BufferAddr)
{
   //debug("cIptvProtocolFile::Read()\n");
   *BufferAddr = readBuffer;
   // Check errors
   if (ferror(fileStream)) {
      debug("Read error\n");
      return -1;
      }
   // Rewind if EOF
   if (feof(fileStream))
      rewind(fileStream);
   // Sleep before reading the file stream to prevent aggressive busy looping
   cCondWait::SleepMs(1);
   // This check is to prevent a race condition where file may be switched off
   // during the sleep and buffers are disposed. Check here that the plugin is
   // still active before accessing the buffers
   if (fileActive)
      return fread(readBuffer, sizeof(unsigned char), (TS_SIZE * IptvConfig.GetFileBufferSize()), fileStream);
   return -1;
}

bool cIptvProtocolFile::Open(void)
{
  debug("cIptvProtocolFile::Open(): streamAddr=%s\n", streamAddr);
  // Open the file stream
  OpenFile();
  return true;
}

bool cIptvProtocolFile::Close(void)
{
  debug("cIptvProtocolFile::Close(): streamAddr=%s\n", streamAddr);
  // Close the file stream
  CloseFile();
  return true;
}

bool cIptvProtocolFile::Set(const char* Address, const int Port)
{
  debug("cIptvProtocolFile::Set(): %s:%d\n", Address, Port);
  if (!isempty(Address)) {
     // Close the file stream
     CloseFile();
     // Update stream address and port
     streamAddr = strcpyrealloc(streamAddr, Address);
     streamPort = Port;
     // Open the file for input
     OpenFile();
     }
  return true;
}
