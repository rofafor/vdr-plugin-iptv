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
: streamAddrM(strdup("")),
  streamPathM(strdup("/")),
  streamPortM(0)
{
  debug("cIptvProtocolHttp::cIptvProtocolHttp()\n");
}

cIptvProtocolHttp::~cIptvProtocolHttp()
{
  debug("cIptvProtocolHttp::~cIptvProtocolHttp()\n");
  // Close the socket
  cIptvProtocolHttp::Close();
  // Free allocated memory
  free(streamPathM);
  free(streamAddrM);
}

bool cIptvProtocolHttp::Connect(void)
{
  debug("cIptvProtocolHttp::Connect()\n");
  // Check that stream address is valid
  if (!isActive && !isempty(streamAddrM) && !isempty(streamPathM)) {
     // Ensure that socket is valid and connect
     OpenSocket(streamPortM, streamAddrM);
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
                                       "\r\n", streamPathM, streamAddrM,
                                       PLUGIN_NAME_I18N, VERSION);
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

bool cIptvProtocolHttp::GetHeaderLine(char* destP, unsigned int destLenP,
                                      unsigned int &recvLenP)
{
  debug("cIptvProtocolHttp::GetHeaderLine()\n");
  bool linefeed = false;
  bool newline = false;
  char *bufptr = destP;
  recvLenP = 0;

  if (!destP)
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
          ++recvLenP;
          }
       ++bufptr;
       // Check that buffer won't be exceeded
       if (recvLenP >= destLenP) {
          error("Header wouldn't fit into buffer\n");
          recvLenP = 0;
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
  snprintf(fmt, sizeof(fmt), "HTTP/1.%%%zui %%%zui ", sizeof(version) - 1, sizeof(response) - 1);

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

int cIptvProtocolHttp::Read(unsigned char* bufferAddrP, unsigned int bufferLenP)
{
  return cIptvTcpSocket::Read(bufferAddrP, bufferLenP);
}

bool cIptvProtocolHttp::Set(const char* locationP, const int parameterP, const int indexP)
{
  debug("cIptvProtocolHttp::Set('%s', %d, %d)\n", locationP, parameterP, indexP);
  if (!isempty(locationP)) {
     // Disconnect the current socket
     Disconnect();
     // Update stream address, path and port
     streamAddrM = strcpyrealloc(streamAddrM, locationP);
     char *path = strstr(streamAddrM, "/");
     if (path) {
        streamPathM = strcpyrealloc(streamPathM, path);
        *path = 0;
        }
     else
        streamPathM = strcpyrealloc(streamPathM, "/");
     streamPortM = parameterP;
     //debug("http://%s:%d%s\n", streamAddrM, streamPortM, streamPathM);
     // Re-connect the socket
     Connect();
     }
  return true;
}

cString cIptvProtocolHttp::GetInformation(void)
{
  //debug("cIptvProtocolHttp::GetInformation()");
  return cString::sprintf("http://%s:%d%s", streamAddrM, streamPortM, streamPathM);
}
