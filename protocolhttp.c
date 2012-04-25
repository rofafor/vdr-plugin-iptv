/*
 * protocolhttp.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <vdr/device.h>

#include "common.h"
#include "config.h"
#include "protocolhttp.h"

cIptvProtocolHttp::cIptvProtocolHttp()
: streamAddr(strdup("")),
  streamPath(strdup("/")),
  streamPort(0)
{
  debug("cIptvProtocolHttp::cIptvProtocolHttp()\n");
}

cIptvProtocolHttp::~cIptvProtocolHttp()
{
  debug("cIptvProtocolHttp::~cIptvProtocolHttp()\n");
  // Close the socket
  cIptvProtocolHttp::Close();
  // Free allocated memory
  free(streamPath);
  free(streamAddr);
}

bool cIptvProtocolHttp::Connect(void)
{
  debug("cIptvProtocolHttp::Connect()\n");
  // Check that stream address is valid
  if (!isActive && !isempty(streamAddr) && !isempty(streamPath)) {
     // Ensure that socket is valid and connect
     OpenSocket(streamPort, streamAddr);
     if (!ConnectSocket()) {
        CloseSocket();
        return false;
        }
     // Formulate and send HTTP request
     cString buffer = cString::sprintf("GET %s HTTP/1.1\r\n"
                                       "Host: %s\r\n"
                                       "User-Agent: vdr-%s/%s\r\n"
                                       "Range: bytes=0-\r\n"
                                       "Connection: Close\r\n"
                                       "\r\n", streamPath, streamAddr, PLUGIN_NAME_I18N, VERSION);
     debug("Sending http request: %s\n", *buffer);
     if (!Write(*buffer, (unsigned int)strlen(*buffer))) {
        CloseSocket();
        return false;
        }
     // Now process headers
     if (!ProcessHeaders()) {
        CloseSocket();
        return false;
        }
     // Update active flag
     isActive = true;
     }
  return true;
}

bool cIptvProtocolHttp::Disconnect(void)
{
  debug("cIptvProtocolHttp::Disconnect()\n");
  if (isActive) {
     // Close the socket
     CloseSocket();
     // Update active flag
     isActive = false;
     }
  return true;
}

bool cIptvProtocolHttp::GetHeaderLine(char* dest, unsigned int destLen,
                                      unsigned int &recvLen)
{
  debug("cIptvProtocolHttp::GetHeaderLine()\n");
  bool linefeed = false;
  bool newline = false;
  char *bufptr = dest;
  recvLen = 0;

  if (!dest)
     return false;

  while (!newline || !linefeed) {
    // Wait 500ms for data
    if (ReadChar(bufptr, 500)) {
       // Parsing end conditions, if line ends with \r\n
       if (linefeed && *bufptr == '\n')
          newline = true;
       // First occurrence of \r seen
       else if (*bufptr == '\r')
          linefeed = true;
       // Saw just data or \r without \n
       else {
          linefeed = false;
          ++recvLen;
          }
       ++bufptr;
       // Check that buffer won't be exceeded
       if (recvLen >= destLen) {
          error("Header wouldn't fit into buffer\n");
          recvLen = 0;
          return false;
          }
       }
    else {
       error("No HTTP response received in 500ms\n");
       return false;
       }
    }
  return true;
}

bool cIptvProtocolHttp::ProcessHeaders(void)
{
  debug("cIptvProtocolHttp::ProcessHeaders()\n");
  unsigned int lineLength = 0;
  int version = 0, response = 0;
  bool responseFound = false;
  char fmt[32];
  char buf[4096];

  // Generate HTTP response format string with 2 arguments
  snprintf(fmt, sizeof(fmt), "HTTP/1.%%%ldi %%%ldi ", sizeof(version) - 1, sizeof(response) - 1);

  while (!responseFound || lineLength != 0) {
    memset(buf, '\0', sizeof(buf));
    if (!GetHeaderLine(buf, sizeof(buf), lineLength))
       return false;
    if (!responseFound && sscanf(buf, fmt, &version, &response) != 2) {
       error("Expected HTTP header not found\n");
       continue;
       }
    else
       responseFound = true;
    // Allow only 'OK' and 'Partial Content'
    if ((response != 200) && (response != 206)) {
       error("Invalid HTTP response (%d): %s\n", response, buf);
       return false;
       }
    }
  return true;
}

bool cIptvProtocolHttp::Open(void)
{
  debug("cIptvProtocolHttp::Open()\n");
  // Connect the socket
  return Connect();
}

bool cIptvProtocolHttp::Close(void)
{
  debug("cIptvProtocolHttp::Close()\n");
  // Disconnect the current stream
  Disconnect();
  return true;
}

int cIptvProtocolHttp::Read(unsigned char* BufferAddr, unsigned int BufferLen)
{
  return cIptvTcpSocket::Read(BufferAddr, BufferLen);
}

bool cIptvProtocolHttp::Set(const char* Location, const int Parameter, const int Index)
{
  debug("cIptvProtocolHttp::Set(): Location=%s Parameter=%d Index=%d\n", Location, Parameter, Index);
  if (!isempty(Location)) {
     // Disconnect the current socket
     Disconnect();
     // Update stream address, path and port
     streamAddr = strcpyrealloc(streamAddr, Location);
     char *path = strstr(streamAddr, "/");
     if (path) {
        streamPath = strcpyrealloc(streamPath, path);
        *path = 0;
        }
     else
        streamPath = strcpyrealloc(streamPath, "/");
     streamPort = Parameter;
     //debug("http://%s:%d%s\n", streamAddr, streamPort, streamPath);
     // Re-connect the socket
     Connect();
     }
  return true;
}

cString cIptvProtocolHttp::GetInformation(void)
{
  //debug("cIptvProtocolHttp::GetInformation()");
  return cString::sprintf("http://%s:%d%s", streamAddr, streamPort, streamPath);
}
