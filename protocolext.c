/*
 * protocolext.c: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
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
#include "log.h"
#include "protocolext.h"

#ifndef EXTSHELL
#define EXTSHELL "/bin/bash"
#endif

cIptvProtocolExt::cIptvProtocolExt()
: pidM(-1),
  scriptFileM(""),
  scriptParameterM(0),
  streamPortM(0)
{
  debug1("%s", __PRETTY_FUNCTION__);
}

cIptvProtocolExt::~cIptvProtocolExt()
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Drop the socket connection
  cIptvProtocolExt::Close();
}

void cIptvProtocolExt::ExecuteScript(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Check if already executing
  if (isActiveM || isempty(scriptFileM))
     return;
  if (pidM > 0) {
     error("Cannot execute script!");
     return;
     }
  // Let's fork
  ERROR_IF_RET((pidM = fork()) == -1, "fork()", return);
  // Check if child process
  if (pidM == 0) {
     // Close all dup'ed filedescriptors
     int MaxPossibleFileDescriptors = getdtablesize();
     for (int i = STDERR_FILENO + 1; i < MaxPossibleFileDescriptors; i++)
         close(i);
     // Execute the external script
     cString cmd = cString::sprintf("%s %d %d", *scriptFileM, scriptParameterM, streamPortM);
     debug1("%s Child %s", __PRETTY_FUNCTION__, *cmd);
     // Create a new session for a process group
     ERROR_IF_RET(setsid() == -1, "setsid()", _exit(-1));
     if (execl(EXTSHELL, "sh", "-c", *cmd, (char *)NULL) == -1) {
        error("Script execution failed: %s", *cmd);
        _exit(-1);
        }
     _exit(0);
     }
  else {
     debug1("%s pid=%d", __PRETTY_FUNCTION__, pidM);
     }
}

void cIptvProtocolExt::TerminateScript(void)
{
  debug1("%s pid=%d", __PRETTY_FUNCTION__, pidM);
  if (!isActiveM || isempty(scriptFileM))
     return;
  if (pidM > 0) {
     const unsigned int timeoutms = 100;
     unsigned int waitms = 0;
     bool waitOver = false;
     // Signal and wait for termination
     int retval = killpg(pidM, SIGINT);
     ERROR_IF_RET(retval < 0, "kill()", waitOver = true);
     while (!waitOver) {
       retval = 0;
       waitms += timeoutms;
       if ((waitms % 2000) == 0) {
          error("Script '%s' won't terminate - killing it!", *scriptFileM);
          killpg(pidM, SIGKILL);
          }
       // Clear wait status to make sure child exit status is accessible
       // and wait for child termination
#ifdef __FreeBSD__
       int waitStatus = 0;
       retval = waitpid(pidM, &waitStatus, WNOHANG);
#else  // __FreeBSD__
       siginfo_t waitStatus;
       memset(&waitStatus, 0, sizeof(waitStatus));
       retval = waitid(P_PID, pidM, &waitStatus, (WNOHANG | WEXITED));
#endif // __FreeBSD__
       ERROR_IF_RET(retval < 0, "waitid()", waitOver = true);
       // These are the acceptable conditions under which child exit is
       // regarded as successful
#ifdef __FreeBSD__
       if (retval > 0 && (WIFEXITED(waitStatus) || WIFSIGNALED(waitStatus))) {
#else  // __FreeBSD__
       if (!retval && waitStatus.si_pid && (waitStatus.si_pid == pidM) &&
          ((waitStatus.si_code == CLD_EXITED) || (waitStatus.si_code == CLD_KILLED))) {
#endif // __FreeBSD__
          debug1("%s Child (%d) exited as expected", __PRETTY_FUNCTION__, pidM);
          waitOver = true;
          }
       // Unsuccessful wait, avoid busy looping
       if (!waitOver)
          cCondWait::SleepMs(timeoutms);
       }
     pidM = -1;
     }
}

bool cIptvProtocolExt::Open(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Reject empty script files
  if (!strlen(*scriptFileM))
     return false;
  // Create the listening socket
  OpenSocket(streamPortM);
  // Execute the external script
  ExecuteScript();
  isActiveM = true;
  return true;
}

bool cIptvProtocolExt::Close(void)
{
  debug1("%s", __PRETTY_FUNCTION__);
  // Terminate the external script
  TerminateScript();
  isActiveM = false;
  // Close the socket
  CloseSocket();
  return true;
}

int cIptvProtocolExt::Read(unsigned char* bufferAddrP, unsigned int bufferLenP)
{
  return cIptvUdpSocket::Read(bufferAddrP, bufferLenP);
}

bool cIptvProtocolExt::SetSource(const char* locationP, const int parameterP, const int indexP)
{
  debug1("%s (%s, %d, %d)", __PRETTY_FUNCTION__, locationP, parameterP, indexP);
  if (!isempty(locationP)) {
     struct stat stbuf;
     // Update script file and parameter
     scriptFileM = cString::sprintf("%s/%s", IptvConfig.GetResourceDirectory(), locationP);
     if ((stat(*scriptFileM, &stbuf) != 0) || (strstr(*scriptFileM, "..") != 0)) {
        error("Non-existent or relative path script '%s'", *scriptFileM);
        return false;
        }
     scriptParameterM = parameterP;
     // Update listen port
     streamPortM = IptvConfig.GetProtocolBasePort() + indexP * 2;
     }
  return true;
}

bool cIptvProtocolExt::SetPid(int pidP, int typeP, bool onP)
{
  debug16("%s (%d, %d, %d)", __PRETTY_FUNCTION__, pidP, typeP, onP);
  return true;
}

cString cIptvProtocolExt::GetInformation(void)
{
  debug16("%s", __PRETTY_FUNCTION__);
  return cString::sprintf("ext://%s:%d", *scriptFileM, scriptParameterM);
}
