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
: isIGMPv3(false),
  sourceAddr(strdup("")),
  streamAddr(strdup("")),
  streamPort(0)
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
  debug("cIptvProtocolUdp::Open(): streamAddr=%s\n", streamAddr);
  OpenSocket(streamPort, streamAddr, sourceAddr, isIGMPv3);
  if (!isempty(streamAddr)) {
     // Join a new multicast group
     JoinMulticast();
     }
  return true;
}

bool cIptvProtocolUdp::Close(void)
{
  debug("cIptvProtocolUdp::Close(): streamAddr=%s\n", streamAddr);
  if (!isempty(streamAddr)) {
     // Drop the multicast group
     OpenSocket(streamPort, streamAddr, sourceAddr, isIGMPv3);
     DropMulticast();
     }
  // Close the socket
  CloseSocket();
  // Do NOT reset stream and source addresses
  //sourceAddr = strcpyrealloc(sourceAddr, "");
  //streamAddr = strcpyrealloc(streamAddr, "");
  //streamPort = 0;
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
        OpenSocket(streamPort, streamAddr, sourceAddr, isIGMPv3);
        DropMulticast();
        }
     // Update stream address and port
     streamAddr = strcpyrealloc(streamAddr, Location);
     // <group address> or <source address>@<group address>
     char *p = strstr(streamAddr, "@");
     if (p) {
        *p = 0;
        sourceAddr = strcpyrealloc(sourceAddr, streamAddr);
        streamAddr = strcpyrealloc(streamAddr, p + 1);
        isIGMPv3 = true;
        }
     else {
        sourceAddr = strcpyrealloc(sourceAddr, streamAddr);
        isIGMPv3 = false;
        }
     streamPort = Parameter;
     // Join a new multicast group
     if (!isempty(streamAddr)) {
        OpenSocket(streamPort, streamAddr, sourceAddr, isIGMPv3);
        JoinMulticast();
        }
     }
  return true;
}

cString cIptvProtocolUdp::GetInformation(void)
{
  //debug("cIptvProtocolUdp::GetInformation()");
  if (isIGMPv3)
     return cString::sprintf("udp://%s@%s:%d", sourceAddr, streamAddr, streamPort);
  return cString::sprintf("udp://%s:%d", streamAddr, streamPort);
}
