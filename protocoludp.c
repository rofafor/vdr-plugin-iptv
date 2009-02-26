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
{
  debug("cIptvProtocolUdp::cIptvProtocolUdp()\n");
  streamAddr = strdup("");
}

cIptvProtocolUdp::~cIptvProtocolUdp()
{
  debug("cIptvProtocolUdp::~cIptvProtocolUdp()\n");
  // Drop the multicast group and close the socket
  cIptvProtocolUdp::Close();
  // Free allocated memory
  free(streamAddr);
}

bool cIptvProtocolUdp::JoinMulticast(void)
{
  debug("cIptvProtocolUdp::JoinMulticast()\n");
  // Check that stream address is valid
  if (!isActive && !isempty(streamAddr)) {
     // Ensure that socket is valid
     OpenSocket(socketPort);
     // Join a new multicast group
     struct ip_mreq mreq;
     mreq.imr_multiaddr.s_addr = inet_addr(streamAddr);
     mreq.imr_interface.s_addr = htonl(INADDR_ANY);
     int err = setsockopt(socketDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                          sizeof(mreq));
     ERROR_IF_RET(err < 0, "setsockopt()", return false);
     // Update multicasting flag
     isActive = true;
     }
  return true;
}

bool cIptvProtocolUdp::DropMulticast(void)
{
  debug("cIptvProtocolUdp::DropMulticast()\n");
  // Check that stream address is valid
  if (isActive && !isempty(streamAddr)) {
      // Ensure that socket is valid
      OpenSocket(socketPort);
      // Drop the multicast group
      struct ip_mreq mreq;
      mreq.imr_multiaddr.s_addr = inet_addr(streamAddr);
      mreq.imr_interface.s_addr = htonl(INADDR_ANY);
      int err = setsockopt(socketDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,
                           sizeof(mreq));
      ERROR_IF_RET(err < 0, "setsockopt()", return false);
      // Update multicasting flag
      isActive = false;
     }
  return true;
}

bool cIptvProtocolUdp::Open(void)
{
  debug("cIptvProtocolUdp::Open()\n");
  // Join a new multicast group
  JoinMulticast();
  return true;
}

bool cIptvProtocolUdp::Close(void)
{
  debug("cIptvProtocolUdp::Close()\n");
  // Drop the multicast group
  DropMulticast();
  // Close the socket
  CloseSocket();
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
    DropMulticast();
    // Update stream address and port
    streamAddr = strcpyrealloc(streamAddr, Location);
    socketPort = Parameter;
    // Join a new multicast group
    JoinMulticast();
    }
  return true;
}

cString cIptvProtocolUdp::GetInformation(void)
{
  //debug("cIptvProtocolUdp::GetInformation()");
  return cString::sprintf("udp://%s:%d", streamAddr, socketPort);
}
