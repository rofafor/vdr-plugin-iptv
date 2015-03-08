/*
 * log.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#ifndef __IPTV_LOG_H
#define __IPTV_LOG_H

#include "config.h"

#define error(x...)   esyslog("IPTV-ERROR: " x)
#define info(x...)    isyslog("IPTV: " x)
// 0x0001: Generic call stack
#define debug1(x...)  void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug1)  ? dsyslog("IPTV1: " x)  : void() )
// 0x0002: TBD
#define debug2(x...)  void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug2)  ? dsyslog("IPTV2: " x)  : void() )
// 0x0004: TBD
#define debug3(x...)  void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug3)  ? dsyslog("IPTV3: " x)  : void() )
// 0x0008: TBD
#define debug4(x...)  void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug4)  ? dsyslog("IPTV4: " x)  : void() )
// 0x0010: TBD
#define debug5(x...)  void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug5)  ? dsyslog("IPTV5: " x)  : void() )
// 0x0020: TBD
#define debug6(x...)  void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug6)  ? dsyslog("IPTV6: " x)  : void() )
// 0x0040: TBD
#define debug7(x...)  void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug7)  ? dsyslog("IPTV7: " x)  : void() )
// 0x0080: CURL
#define debug8(x...)  void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug8)  ? dsyslog("IPTV8: " x)  : void() )
// 0x0100: TBD
#define debug9(x...)  void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug9)  ? dsyslog("IPTV9: " x)  : void() )
// 0x0200: TBD
#define debug10(x...) void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug10) ? dsyslog("IPTV10: " x) : void() )
// 0x0400: TBD
#define debug11(x...) void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug11) ? dsyslog("IPTV11: " x) : void() )
// 0x0800: TBD
#define debug12(x...) void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug12) ? dsyslog("IPTV12: " x) : void() )
// 0x1000: TBD
#define debug13(x...) void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug13) ? dsyslog("IPTV13: " x) : void() )
// 0x2000: TBD
#define debug14(x...) void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug14) ? dsyslog("IPTV14: " x) : void() )
// 0x4000: TBD
#define debug15(x...) void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug15) ? dsyslog("IPTV15: " x) : void() )
// 0x8000; Extra call stack
#define debug16(x...) void( IptvConfig.IsTraceMode(cIptvConfig::eTraceModeDebug16) ? dsyslog("IPTV16: " x) : void() )

#endif // __IPTV_LOG_H
