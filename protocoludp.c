/*
 * protocoludp.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>

#include "common.h"
#include "config.h"
#include "protocoludp.h"
#include "socket.h"

cIptvProtocolUdp::cIptvProtocolUdp()
: streamAddr(strdup("")),
  sourceAddr(strdup(""))
{
  debug("cIptvProtocolUdp::cIptvProtocolUdp()\n");
}

cIptvProtocolUdp::~cIptvProtocolUdp()
{
  debug("cIptvProtocolUdp::~cIptvProtocolUdp()\n");
  // Drop the multicast group and close the socket
  cIptvProtocolUdp::Close();
  // Free allocated memory
  free(streamAddr);
  free(sourceAddr);
}

bool cIptvProtocolUdp::Open(void)
{
  debug("cIptvProtocolUdp::Open(): sourceAddr=%s streamAddr=%s\n", sourceAddr, streamAddr);
  OpenSocket(socketPort, isempty(sourceAddr) ? INADDR_ANY : inet_addr(sourceAddr));
  if (!isempty(streamAddr)) {
     // Join a new multicast group
     JoinMulticast(inet_addr(streamAddr));
     }
  return true;
}

bool cIptvProtocolUdp::Close(void)
{
  debug("cIptvProtocolUdp::Close(): sourceAddr=%s streamAddr=%s\n", sourceAddr, streamAddr);
  if (!isempty(streamAddr)) {
     // Drop the multicast group
     OpenSocket(socketPort, isempty(sourceAddr) ? INADDR_ANY : inet_addr(sourceAddr));
     DropMulticast(inet_addr(streamAddr));
     }
  // Close the socket
  CloseSocket();
  // Do NOT reset stream and source addresses
  //streamAddr = strcpyrealloc(streamAddr, "");
  //sourceAddr = strcpyrealloc(sourceAddr, "");
  return true;
}

int cIptvProtocolUdp::Read(unsigned char* BufferAddr, unsigned int BufferLen)
{
  return cIptvUdpSocket::Read(BufferAddr, BufferLen);
}

bool cIptvProtocolUdp::Set(const char* Location, const int Parameter, const int Index)
{
  debug("cIptvProtocolUdp::Set(): Location=%s Parameter=%d Index=%d\n", Location, Parameter, Index);
  if (!isempty(Location)) {
     // Drop the multicast group
     if (!isempty(streamAddr)) {
        OpenSocket(socketPort, isempty(sourceAddr) ? INADDR_ANY : inet_addr(sourceAddr));
        DropMulticast(inet_addr(streamAddr));
        }
     // Update stream address and port
     streamAddr = strcpyrealloc(streamAddr, Location);
     char *p = strstr(streamAddr, ";");
     if (p) {
        sourceAddr = strcpyrealloc(sourceAddr, p + 1);
        *p = 0;
        }
     else
        sourceAddr = strcpyrealloc(sourceAddr, "");
     socketPort = Parameter;
     // Join a new multicast group
     if (!isempty(streamAddr)) {
        OpenSocket(socketPort, isempty(sourceAddr) ? INADDR_ANY : inet_addr(sourceAddr));
        JoinMulticast(inet_addr(streamAddr));
        }
     }
  return true;
}

cString cIptvProtocolUdp::GetInformation(void)
{
  //debug("cIptvProtocolUdp::GetInformation()");
  return cString::sprintf("udp://%s:%d", streamAddr, socketPort);
}
