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
{
  debug("cIptvProtocolHttp::cIptvProtocolHttp()\n");
  streamAddr = strdup("");
  streamPath = strdup("/");
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
     // Ensure that socket is valid
     OpenSocket(socketPort);

     // First try only the IP address
     sockAddr.sin_addr.s_addr = inet_addr(streamAddr);

     if (sockAddr.sin_addr.s_addr == INADDR_NONE) {
        debug("Cannot convert %s directly to internet address\n", streamAddr);

        // It may be a host name, get the name
        struct hostent *host;
        host = gethostbyname(streamAddr);
        if (!host) {
           char tmp[64];
           error("%s is not valid address: %s", streamAddr, strerror_r(h_errno, tmp, sizeof(tmp)));
           return false;
           }

        sockAddr.sin_addr.s_addr = inet_addr(*host->h_addr_list);
        }

     int err = connect(socketDesc, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
     // Non-blocking sockets always report in-progress error when connected
     ERROR_IF_FUNC(err < 0 && errno != EINPROGRESS, "connect()", CloseSocket(), return false);
     // Select with 800ms timeout on the socket completion, check if it is writable
     int retval = select_single_desc(socketDesc, 800000, true);
     if (retval < 0)
        return retval;

     // Select has returned. Get socket errors if there are any
     int socketStatus = 0;
     socklen_t len = sizeof(socketStatus);
     getsockopt(socketDesc, SOL_SOCKET, SO_ERROR, &socketStatus, &len);

     // If not any errors, then socket must be ready and connected
     if (socketStatus != 0) {
        char tmp[64];
        error("Cannot connect to %s: %s", streamAddr, strerror_r(socketStatus, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }

     // Formulate and send HTTP request
     cString buffer = cString::sprintf("GET %s HTTP/1.1\r\n"
                                       "Host: %s\r\n"
                                       "User-Agent: vdr-iptv\r\n"
                                       "Range: bytes=0-\r\n"
                                       "Connection: Close\r\n"
                                       "\r\n", streamPath, streamAddr);

     debug("Sending http request: %s\n", *buffer);
     ssize_t err2 = send(socketDesc, buffer, strlen(buffer), 0);
     ERROR_IF_FUNC(err2 < 0, "send()", CloseSocket(), return false);

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
  // Check that stream address is valid
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
  char buf[4096];
  char *bufptr = buf;
  memset(buf, '\0', sizeof(buf));
  recvLen = 0;

  while (!newline || !linefeed) {
    socklen_t addrlen = sizeof(sockAddr);
    // Wait 500ms for data
    int retval = select_single_desc(socketDesc, 500000, false);
    // Check if error
    if (retval < 0)
       return false;
    // Check if data available
    else if (retval) {
       ssize_t retval = recvfrom(socketDesc, bufptr, 1, MSG_DONTWAIT,
                             (struct sockaddr *)&sockAddr, &addrlen);
       if (retval <= 0)
          return false;
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
       // Check that buffers won't be exceeded
       if (recvLen >= sizeof(buf) || recvLen >= destLen) {
          error("Header wouldn't fit into buffer\n");
          recvLen = 0;
          return false;
          }
       }
    else {
       error("No HTTP response received\n");
       return false;
       }
    }
  memcpy(dest, buf, recvLen);
  return true;
}

bool cIptvProtocolHttp::ProcessHeaders(void)
{
  debug("cIptvProtocolHttp::ProcessHeaders()\n");
  unsigned int lineLength = 0;
  int response = 0;
  bool responseFound = false;
  char buf[4096];

  while (!responseFound || lineLength != 0) {
    memset(buf, '\0', sizeof(buf));
    if (!GetHeaderLine(buf, sizeof(buf), lineLength))
       return false;
    if (!responseFound && sscanf(buf, "HTTP/1.%*i %i ", &response) != 1) {
       error("Expected HTTP header not found\n");
       continue;
       }
    else
       responseFound = true;
    if (response != 200) {
       error("%s\n", buf);
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
     socketPort = Parameter;
     //debug("http://%s:%d%s\n", streamAddr, socketPort, streamPath);
     // Re-connect the socket
     Connect();
     }
  return true;
}

cString cIptvProtocolHttp::GetInformation(void)
{
  //debug("cIptvProtocolHttp::GetInformation()");
  return cString::sprintf("http://%s:%d%s", streamAddr, socketPort, streamPath);
}
