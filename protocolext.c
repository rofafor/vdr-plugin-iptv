/*
 * protocolext.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolext.c,v 1.14 2007/10/20 20:43:22 ajhseppa Exp $
 */

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <vdr/device.h>
#include <vdr/plugin.h>

#include "common.h"
#include "config.h"
#include "protocolext.h"

cIptvProtocolExt::cIptvProtocolExt()
: pid(-1),
  listenPort(IptvConfig.GetExtProtocolBasePort()),
  scriptParameter(0),
  socketDesc(-1),
  readBufferLen(TS_SIZE * IptvConfig.GetReadBufferTsCount())
{
  debug("cIptvProtocolExt::cIptvProtocolExt()\n");
  scriptFile = strdup("");
  listenAddr = strdup("127.0.0.1");
  // Allocate receive buffer
  readBuffer = MALLOC(unsigned char, readBufferLen);
  if (!readBuffer)
     error("ERROR: MALLOC() failed in ProtocolExt()");
}

cIptvProtocolExt::~cIptvProtocolExt()
{
  debug("cIptvProtocolExt::~cIptvProtocolExt()\n");
  // Drop the multicast group and close the socket
  Close();
  // Free allocated memory
  free(scriptFile);
  free(listenAddr);
  free(readBuffer);
}

bool cIptvProtocolExt::OpenSocket(void)
{
  debug("cIptvProtocolExt::OpenSocket()\n");
  // Bind to the socket if it is not active already
  if (socketDesc < 0) {
     int yes = 1;     
     // Create socket
     socketDesc = socket(PF_INET, SOCK_DGRAM, 0);
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
     // Bind socket
     memset(&sockAddr, '\0', sizeof(sockAddr));
     sockAddr.sin_family = AF_INET;
     sockAddr.sin_port = htons(listenPort);
     sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
     int err = bind(socketDesc, (struct sockaddr *)&sockAddr, sizeof(sockAddr));
     if (err < 0) {
        char tmp[64];
        error("ERROR: bind(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        CloseSocket();
        return false;
        }
     }
  return true;
}

void cIptvProtocolExt::CloseSocket(void)
{
  debug("cIptvProtocolExt::CloseSocket()\n");
  // Check if socket exists
  if (socketDesc >= 0) {
     close(socketDesc);
     socketDesc = -1;
     }
}

void cIptvProtocolExt::ExecuteCommand(void)
{
  debug("cIptvProtocolExt::ExecuteCommand()\n");
  // Check if already executing
  if (pid > 0) {
     error("ERROR: Cannot execute command!");
     return;
     }
  // Let's fork
  if ((pid = fork()) == -1) {
     char tmp[64];
     error("ERROR: fork(): %s", strerror_r(errno, tmp, sizeof(tmp)));
     return;
     }
  // Check if child process
  if (pid == 0) {
     // Close all dup'ed filedescriptors
     int MaxPossibleFileDescriptors = getdtablesize();
     for (int i = STDERR_FILENO + 1; i < MaxPossibleFileDescriptors; i++)
         close(i);
     // Execute the external script
     char* cmd = NULL;
     asprintf(&cmd, "%s %d %d", scriptFile, scriptParameter, listenPort);
     debug("cIptvProtocolExt::ExecuteCommand(child): %s\n", cmd);
     if (execl("/bin/sh", "sh", "-c", cmd, NULL) == -1) {
        error("ERROR: Command failed: %s", cmd);
        free(cmd);
        _exit(-1);
        }
     free(cmd);
     _exit(0);
     }
  else {
     debug("cIptvProtocolExt::ExecuteCommand(): pid=%d\n", pid);
     }
}

void cIptvProtocolExt::TerminateCommand(void)
{
  debug("cIptvProtocolExt::TerminateCommand(): pid=%d\n", pid);
  if (pid > 0) {
     const unsigned int timeoutms = 100;
     unsigned int waitms = 0;
     siginfo_t waitStatus;
     bool waitOver = false;
     // signal and wait for termination
     int retval = kill(pid, SIGINT);
     if (retval < 0) {
        char tmp[64];
        error("ERROR: kill(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        waitOver = true;
        }
     while (!waitOver) {
       retval = 0;
       waitms += timeoutms;
       if ((waitms % 2000) == 0) {
          error("ERROR: Script '%s' won't terminate - killing it!", scriptFile);
          kill(pid, SIGKILL);
          }
       // Clear wait status to make sure child exit status is accessible
       memset(&waitStatus, '\0', sizeof(waitStatus));
       // Wait for child termination
       retval = waitid(P_PID, pid, &waitStatus, (WNOHANG | WEXITED));
       if (retval < 0) {
          char tmp[64];
          error("ERROR: waitid(): %s", strerror_r(errno, tmp, sizeof(tmp)));
          waitOver = true;
          }
       // These are the acceptable conditions under which child exit is
       // regarded as successful
       if (!retval && waitStatus.si_pid && (waitStatus.si_pid == pid) &&
          ((waitStatus.si_code == CLD_EXITED) || (waitStatus.si_code == CLD_KILLED))) {
          debug("Child (%d) exited as expected\n", pid);
          waitOver = true;
          }
       // Unsuccessful wait, avoid busy looping
       if (!waitOver)
          cCondWait::SleepMs(timeoutms);
       }
     pid = -1;
     }
}

int cIptvProtocolExt::Read(unsigned char* *BufferAddr)
{
  //debug("cIptvProtocolExt::Read()\n");
  // Error out if socket not initialized
  if (socketDesc <= 0) {
    error("ERROR: Invalid socket in %s\n", __FUNCTION__);
    return -1;
  }
  socklen_t addrlen = sizeof(sockAddr);
  // Set argument point to read buffer
  *BufferAddr = readBuffer;
  // Wait for data
  int retval = select_single_desc(socketDesc, 500000, false);
  // Check if error
  if (retval < 0)
     return retval;
  // Check if data available
  else if (retval) {
     // Read data from socket
     int len = 0;
     if (isActive)
        len = recvfrom(socketDesc, readBuffer, readBufferLen, MSG_DONTWAIT,
                       (struct sockaddr *)&sockAddr, &addrlen);
     if (len < 0) {
        char tmp[64];
        error("ERROR: recvfrom(): %s", strerror_r(errno, tmp, sizeof(tmp)));
        return len;
        }
     else if ((len > 0) && (readBuffer[0] == 0x47)) {
        // Set argument point to read buffer
        *BufferAddr = &readBuffer[0];
        return len;
        }
     else if (len > 3) {
        // http://www.networksorcery.com/enp/rfc/rfc2250.txt
        // version
        unsigned int v = (readBuffer[0] >> 6) & 0x03;
        // extension bit
        unsigned int x = (readBuffer[0] >> 4) & 0x01;
        // cscr count
        unsigned int cc = readBuffer[0] & 0x0F;
        // payload type
        unsigned int pt = readBuffer[1] & 0x7F;
        // header lenght
        unsigned int headerlen = (3 + cc) * sizeof(uint32_t);
        // check if extension
        if (x) {
           // extension header length
           unsigned int ehl = (((readBuffer[headerlen + 2] & 0xFF) << 8) | (readBuffer[headerlen + 3] & 0xFF));
           // update header length
           headerlen += (ehl + 1) * sizeof(uint32_t);
           }
        // Check that rtp is version 2, payload type is MPEG2 TS
        // and payload contains multiple of TS packet data
        if ((v == 2) && (pt == 33) && (((len - headerlen) % TS_SIZE) == 0)) {
           // Set argument point to payload in read buffer
           *BufferAddr = &readBuffer[headerlen];
           return (len - headerlen);
           }
        }
     }
  return 0;
}

bool cIptvProtocolExt::Open(void)
{
  debug("cIptvProtocolExt::Open()\n");
  // Reject empty script files
  if (!strlen(scriptFile))
      return false;
  // Create the listening socket
  OpenSocket();
  // Execute the external command
  ExecuteCommand();
  isActive = true;
  return true;
}

bool cIptvProtocolExt::Close(void)
{
  debug("cIptvProtocolExt::Close()\n");
  // Close the socket
  CloseSocket();
  // Terminate the external script
  TerminateCommand();
  isActive = false;
  return true;
}

bool cIptvProtocolExt::Set(const char* Location, const int Parameter, const int Index)
{
  debug("cIptvProtocolExt::Set(): Location=%s Parameter=%d Index=%d\n", Location, Parameter, Index);
  if (!isempty(Location)) {
     struct stat stbuf;
     // Update script file and parameter
     free(scriptFile);
     asprintf(&scriptFile, "%s/%s", IptvConfig.GetConfigDirectory(), Location);
     if ((stat(scriptFile, &stbuf) != 0) || (strstr(scriptFile, "..") != 0)) {
        error("ERROR: Non-existent or relative path script '%s'", scriptFile);
        return false;
        }
     scriptParameter = Parameter;
     // Update listen port
     listenPort = IptvConfig.GetExtProtocolBasePort() + Index;
     }
  return true;
}

cString cIptvProtocolExt::GetInformation(void)
{
  //debug("cIptvProtocolExt::GetInformation()");
  return cString::sprintf("ext://%s:%d", scriptFile, scriptParameter);
}
