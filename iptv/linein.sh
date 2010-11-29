#!/bin/sh

# linein.sh is used by the VDR iptv plugin to transcode line-in of
# a soundcard.
#
# The script originates from:
# http://www.vdr-wiki.de/wiki/index.php/Iptv-plugin
#
# An example channels.conf entry:
# linein;IPTV:5:S=0|P=0|F=EXT|U=linein.sh|A=0:I:27500:0:256:0:0:5:5:5:0
#
# linein.sh is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this package; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
# MA 02110-1301, USA.
#

if [ $# -ne 2 ]; then
    logger "$0: error: Invalid parameter count '$#' $*"
    exit 1
fi

# Channels.conf parameter
PARAMETER=${1}

# Iptv plugin listens this port
PORT=${2}

# Stream configuration
TITLE="linein"

# Stream temporary files
LOG=/dev/null

{
# PID 0x100/256 = Audio
arecord -q -D hw:0,0 -f dat | \
ffmpeg -v -1 \
  -f wav \
  -i - \
  -title "${TITLE}" \
  -f mpegts -acodec mp2 -ac 2 -ab 128k -ar 48000 \
  "udp://127.0.0.1:${PORT}?pkt_size=16356"
} > ${LOG} 2>&1
