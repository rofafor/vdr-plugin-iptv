/*
 * protocolext.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: protocolext.c,v 1.21 2008/01/04 23:36:37 ajhseppa Exp $
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
  scriptParameter(0)
{
  debug("cIptvProtocolExt::cIptvProtocolExt()\n");
  scriptFile = strdup("");
  listenAddr = strdup("127.0.0.1");
}

cIptvProtocolExt::~cIptvProtocolExt()
{
  debug("cIptvProtocolExt::~cIptvProtocolExt()\n");
  // Drop the socket connection
  cIptvProtocolExt::Close();
  // Free allocated memory
  free(scriptFile);
  free(listenAddr);
}

void cIptvProtocolExt::ExecuteScript(void)
{
  debug("cIptvProtocolExt::ExecuteScript()\n");
  // Check if already executing
  if (pid > 0) {
     error("ERROR: Cannot execute script!");
     return;
     }
  // Let's fork
  ERROR_IF_RET((pid = fork()) == -1, "fork()", return);
  // Check if child process
  if (pid == 0) {
     // Close all dup'ed filedescriptors
     int MaxPossibleFileDescriptors = getdtablesize();
     for (int i = STDERR_FILENO + 1; i < MaxPossibleFileDescriptors; i++)
         close(i);
     // Execute the external script
     char* cmd = NULL;
     asprintf(&cmd, "%s %d %d", scriptFile, scriptParameter, socketPort);
     debug("cIptvProtocolExt::ExecuteScript(child): %s\n", cmd);
     if (execl("/bin/sh", "sh", "-c", cmd, NULL) == -1) {
        error("ERROR: Script executionfailed: %s", cmd);
        free(cmd);
        _exit(-1);
        }
     free(cmd);
     _exit(0);
     }
  else {
     debug("cIptvProtocolExt::ExecuteScript(): pid=%d\n", pid);
     }
}

void cIptvProtocolExt::TerminateScript(void)
{
  debug("cIptvProtocolExt::TerminateScript(): pid=%d\n", pid);
  if (pid > 0) {
     const unsigned int timeoutms = 100;
     unsigned int waitms = 0;
     siginfo_t waitStatus;
     bool waitOver = false;
     // signal and wait for termination
     int retval = kill(pid, SIGINT);
     ERROR_IF_RET(retval < 0, "kill()", waitOver = true);
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
       ERROR_IF_RET(retval < 0, "waitid()", waitOver = true);
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

bool cIptvProtocolExt::Open(void)
{
  debug("cIptvProtocolExt::Open()\n");
  // Reject empty script files
  if (!strlen(scriptFile))
      return false;
  // Create the listening socket
  OpenSocket(socketPort);
  // Execute the external script
  ExecuteScript();
  isActive = true;
  return true;
}

bool cIptvProtocolExt::Close(void)
{
  debug("cIptvProtocolExt::Close()\n");
  // Close the socket
  CloseSocket();
  // Terminate the external script
  TerminateScript();
  isActive = false;
  return true;
}

int cIptvProtocolExt::Read(unsigned char* *BufferAddr)
{
  return cIptvUdpSocket::Read(BufferAddr);
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
     socketPort = IptvConfig.GetExtProtocolBasePort() + Index;
     }
  return true;
}

cString cIptvProtocolExt::GetInformation(void)
{
  //debug("cIptvProtocolExt::GetInformation()");
  return cString::sprintf("ext://%s:%d", scriptFile, scriptParameter);
}
