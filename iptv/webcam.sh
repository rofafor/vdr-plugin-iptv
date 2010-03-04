#!/bin/sh

# webcam.sh is used by the VDR iptv plugin to transcode an internet radio
# stream remuxed with images (e.g. webcam) to provide a video stream.
#
# The script originates from:
# http://www.vdr-wiki.de/wiki/index.php/Iptv-plugin
#
# An example channels.conf entry:
# webcam;IPTV:3:S=0|P=0|F=EXT|U=webcam.sh|A=0:I:0:256:257:0:0:3:0:0:0
#
# webcam.sh is free software; you can redistribute it and/or modify
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
URL="http://stream.address"
IMAGE="image.jpg"
TITLE="webcam"

# Stream temporary files
FIFO=/tmp/webcam.fifo
LOG=/dev/null

{
rm -f "${FIFO}"
mkfifo "${FIFO}"

mplayer -dumpstream "${URL}" \
  -quiet -nolirc -noautosub -noconsolecontrols -novideo -nojoystick \
  -dumpfile "${FIFO}" &

# Time to connect and fill pipe
sleep 3 

# Build stream from audiodump with cycle image as video
# PID 0x100/256 = Video 0x101/257 = Audio
ffmpeg -v -1 \
  -i "${FIFO}" -r 0.5 -loop_input -i '${IMAGE}' \
  -title "${TITLE}" \
  -f mpegts -intra -r 24 -vcodec mpeg2video -b 500k -s 352x288 \
  -acodec mp2 -ac 2 -ab 96k -ar 48000 \
  "udp://127.0.0.1:${PORT}?pkt_size=16356"

rm -f "${FIFO}"
} > ${LOG} 2>&1
