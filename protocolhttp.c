/*
 * protocolhttp.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolhttp.c,v 1.11 2007/10/19 21:36:28 rahrenbe Exp $
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
: streamPort(3000),
  socketDesc(-1),
  readBufferLen(TS_SIZE * IptvConfig.GetReadBufferTsCount()),
  isActive(false)
{
  debug("cIptvProtocolHttp::cIptvProtocolHttp()\n");
  streamAddr = strdup("");
  streamPath = strdup("/");
  // Allocate receive buffer
  readBuffer = MALLOC(unsigned char, readBufferLen);
  if (!readBuffer)
     error("ERROR: MALLOC() failed in ProtocolHttp()");
}

cIptvProtocolHttp::~cIptvProtocolHttp()
{
  debug("cIptvProtocolHttp::~cIptvProtocolHttp()\n");
  // Close the socket
  Close();
  // Free allocated memory
  free(streamPath);
  free(streamAddr);
  free(readBuffer);
}

bool cIptvProtocolHttp::OpenSocket(const int Port)
{
  debug("cIptvProtocolHttp::OpenSocket()\n");
  // If socket is there already and it is bound to a different port, it must
  // be closed first
  if (Port != streamPort) {
     debug("cIptvProtocolHttp::OpenSocket(): Socket tear-down\n");
     CloseSocket();
     }
  // Bind to the socket if it is not active already
  if (socketDesc < 0) {
     int yes = 1;     
     // Create socket
     socketDesc = socket(PF_INET, SOCK_STREAM, 0);
     if (socketDesc < 0) {
        char tmp[64];
        error("ERROR: socket(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return false;
        }

     // Make it use non-blocking I/O to avoid stuck read calls
     if (fcntl(socketDesc, F_SETFL, O_NONBLOCK)) {
        char tmp[64];
        error("ERROR: fcntl(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }

     // Allow multiple sockets to use the same PORT number
     if (setsockopt(socketDesc, SOL_SOCKET, SO_REUSEADDR, &yes,
		    sizeof(yes)) < 0) {
        char tmp[64];
        error("ERROR: setsockopt(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }

     // Create default socket
     memset(&sockAddr, '\0', sizeof(sockAddr));
     sockAddr.sin_family = AF_INET;
     sockAddr.sin_port = htons(Port);
     sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

     // Update stream port
     streamPort = Port;
     }
  return true;
}

void cIptvProtocolHttp::CloseSocket(void)
{
  debug("cIptvProtocolHttp::CloseSocket()\n");
  // Check if socket exists
  if (socketDesc >= 0) {
     close(socketDesc);
     socketDesc = -1;
     }
}

bool cIptvProtocolHttp::Connect(void)
{
  debug("cIptvProtocolHttp::Connect()\n");
  // Check that stream address is valid
  if (!isActive && !isempty(streamAddr) && !isempty(streamPath)) {
     // Ensure that socket is valid
     OpenSocket(streamPort);

     // First try only the IP address
     sockAddr.sin_addr.s_addr = inet_addr(streamAddr);
     
     if (sockAddr.sin_addr.s_addr == INADDR_NONE) {
        debug("Cannot convert %s directly to internet address\n", streamAddr);

        // It may be a host name, get the name
        struct hostent *host;
        host = gethostbyname(streamAddr);
        if (!host) {
           error("%s is not valid address\n", streamAddr);
           char tmp[64];
           error("ERROR: %s", strerror_r(h_errno, tmp, sizeof(tmp)));
           return false;
           }

        sockAddr.sin_addr.s_addr = inet_addr(*host->h_addr_list);
        }

     int err = connect(socketDesc, (struct sockaddr*)&sockAddr,
		       sizeof(sockAddr));
     // Non-blocking sockets always report in-progress error when connected
     if (err < 0 && errno != EINPROGRESS) {
        char tmp[64];
        error("ERROR: Connect(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }

     // Select on the socket completion
     struct timeval tv;
     tv.tv_sec = 1;
     tv.tv_usec = 0;
     // Use select to check socket writability
     fd_set wfds;
     FD_ZERO(&wfds);
     FD_SET(socketDesc, &wfds);
     int retval = select(socketDesc + 1, NULL, &wfds, NULL, &tv);
     // Check if error
     if (retval < 0) {
        char tmp[64];
        error("ERROR: select(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return -1;
        }
     
     // Select has returned. Get socket errors if there are any
     int socketStatus = 0;
     socklen_t len = sizeof(socketStatus);
     getsockopt(socketDesc, SOL_SOCKET, SO_ERROR, &socketStatus, &len);

     // If not any errors, then socket must be ready and connected
     if (socketStatus != 0) {
        error("Cannot connect to %s\n", streamAddr);
        char tmp[64];
        error("ERROR: %s", strerror_r(socketStatus, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }

     // Formulate and send HTTP request
     char buffer[256];
     memset(buffer, '\0', sizeof(buffer));
     snprintf(buffer, sizeof(buffer),
	      "GET %s HTTP/1.1\r\n"
	      "Host: %s\r\n"
	      "User-Agent: vdr-iptv\r\n"
	      "Range: bytes=0-\r\n"
	      "Connection: Close\r\n"
	      "\r\n", streamPath, streamAddr);

     //debug("Sending http request: %s\n", buffer);
     err = send(socketDesc, buffer, strlen(buffer), 0);
     if (err < 0) {
        char tmp[64];
        error("ERROR: send(): %s", strerror_r(errno, tmp, sizeof(tmp)));
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
  char buf[256];
  char *bufptr = buf;
  memset(buf, '\0', sizeof(buf));
  recvLen = 0;

  while (!newline || !linefeed) {
    socklen_t addrlen = sizeof(sockAddr);
    // Set argument point to read buffer
    // Wait for data
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    // Use select
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(socketDesc, &rfds);
    int retval = select(socketDesc + 1, &rfds, NULL, NULL, &tv);
    // Check if error
    if (retval < 0) {
       char tmp[64];
       error("ERROR: select(): %s", strerror_r(errno, tmp, sizeof(tmp)));
       return false;
       }
    // Check if data available
    else if (retval) {
       int retval = recvfrom(socketDesc, bufptr, 1, MSG_DONTWAIT,
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
  char buf[256];

  while (!responseFound || lineLength != 0) {
    memset(buf, '\0', sizeof(buf));
    if (!GetHeaderLine(buf, sizeof(buf), lineLength))
       return false;
    if (!responseFound && sscanf(buf, "HTTP/1.%*i %i ",&response) != 1) {
       error("Expected HTTP header not found\n");
       continue;
       }
    else
       responseFound = true;
    if (response != 200) {
       error("ERROR: %s\n", buf);
       return false;
       }
    }
  return true;
}

int cIptvProtocolHttp::Read(unsigned char* *BufferAddr)
{
  //debug("cIptvProtocolHttp::Read()\n");
  socklen_t addrlen = sizeof(sockAddr);
  // Set argument point to read buffer
  *BufferAddr = readBuffer;
  // Wait for data
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  // Use select
  fd_set rfds;
  FD_ZERO(&rfds);
  FD_SET(socketDesc, &rfds);
  int retval = select(socketDesc + 1, &rfds, NULL, NULL, &tv);
  // Check if error
  if (retval < 0) {
     char tmp[64];
     error("ERROR: select(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     return -1;
     }
  // Check if data available
  else if (retval) {
     // Read data from socket
     return recvfrom(socketDesc, readBuffer, readBufferLen, MSG_DONTWAIT,
                     (struct sockaddr *)&sockAddr, &addrlen);
     }
  return 0;
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

bool cIptvProtocolHttp::Set(const char* Location, const int Parameter)
{
  debug("cIptvProtocolHttp::Set(): Location=%s Parameter=%d\n", Location, Parameter);
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
    debug("http://%s:%d%s\n", streamAddr, streamPort, streamPath);
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
