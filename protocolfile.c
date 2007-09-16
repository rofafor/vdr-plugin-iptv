/*
 * protocolfile.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolfile.c,v 1.2 2007/09/16 13:38:20 rahrenbe Exp $
 */

#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>

#include "common.h"
#include "config.h"
#include "protocolfile.h"

cIptvProtocolFile::cIptvProtocolFile()
: fileDesc(-1),
  readBufferLen(TS_SIZE * IptvConfig.GetFileBufferSize()),
  fileActive(false)
{
  debug("cIptvProtocolFile::cIptvProtocolFile(): readBufferLen=%d (%d)\n",
        readBufferLen, (readBufferLen / TS_SIZE));
  streamAddr = strdup("");
  // Allocate receive buffer
  readBuffer = MALLOC(unsigned char, readBufferLen);
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

     fileDesc = open(streamAddr, O_RDONLY | O_NOCTTY | O_NONBLOCK);
     if (fileDesc < 0) {
        char tmp[64];
        error("ERROR: open(): %s", strerror_r(errno, tmp, sizeof(tmp)));
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
  // Check that stream address is valid
  if (fileActive && !isempty(streamAddr)) {
     close(fileDesc);
     // Update multicasting flag
     fileActive = false;
     }
}

int cIptvProtocolFile::Read(unsigned char* *BufferAddr)
{
   //debug("cIptvProtocolFile::Read()\n");

   return read(fileDesc, readBuffer, readBufferLen);
}

bool cIptvProtocolFile::Open(void)
{
  debug("cIptvProtocolFile::Open(): streamAddr=%s\n", streamAddr);
  // Open the file for input
  OpenFile();
  return true;
}

bool cIptvProtocolFile::Close(void)
{
  debug("cIptvProtocolFile::Close(): streamAddr=%s\n", streamAddr);
  // Close the file input
  CloseFile();
  return true;
}

bool cIptvProtocolFile::Set(const char* Address, const int Port)
{
  debug("cIptvProtocolFile::Set(): %s:%d\n", Address, Port);
  if (!isempty(Address)) {
    // Close the file input
    CloseFile();
    // Update stream address and port
    streamAddr = strcpyrealloc(streamAddr, Address);
    streamPort = Port;
    // Open the file for input
    OpenFile();
    }
  return true;
}
