/*
 * i18n.h: IPTV plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: i18n.h,v 1.1 2007/10/28 16:22:44 rahrenbe Exp $
 */

#ifndef __IPTV_I18N_H
#define __IPTV_I18N_H

#include <vdr/i18n.h>

#if defined(APIVERSNUM) && APIVERSNUM < 10507
extern const tI18nPhrase IptvPhrases[];
#endif

#endif // __IPTV_I18N_H
