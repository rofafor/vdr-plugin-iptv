#!/bin/sh

set -e

# image.sh is used by the VDR iptv plugin to transcode images from
# a web server to provide a video stream.
#
# The script originates from Peter Holik
#
# Example channels.conf entries:
# Energy;IPTV:50:S=0|P=0|F=EXT|U=png.sh|A=1:I:0:256:257:0:0:3:0:0:0
# Temperature;IPTV:60:S=0|P=0|EXT|U=png.sh|A=2:I:0:256:257:0:0:3:0:0:0
# Temperature Week;IPTV:70:S=0|P=0|EXT|U=png.sh|A=3:I:0:256:257:0:0:3:0:0:0
# Server Temperature;IPTV:80:S=0|P=0|EXT|U=png.sh|A=4:I:0:256:257:0:0:3:0:0:0
# Server Temperature Week;IPTV:90:S=0|P=0|EXT|U=png.sh|A=5:I:0:256:257:0:0:3:0:0:0
# Traffic;IPTV:100:S=0|P=0|EXT|U=png.sh|A=6:I:0:256:257:0:0:3:0:0:0
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
case ${1} in
    1)
        URL="http://proxy/cgi-bin/bin/graph.cgi?hostname=proxy;plugin=energy;type=electricity"
        ;;
    2)
        URL="http://proxy/cgi-bin/bin/graph.cgi?hostname=proxy;plugin=temp;type=temperature;type_instance=Outdoor"
        ;;
    3)
        URL="http://proxy/cgi-bin/bin/graph.cgi?hostname=proxy;plugin=temp;type=temperature;type_instance=Outdoor;begin=-604800"
        ;;
    4)
        URL="http://proxy/cgi-bin/bin/graph.cgi?hostname=proxy;plugin=temp;type=temperature;type_instance=Server"
        ;;
    5)
        URL="http://proxy/cgi-bin/bin/graph.cgi?hostname=proxy;plugin=temp;type=temperature;type_instance=Server;begin=-604800"
        ;;
    6)
        URL="http://proxy/cgi-bin/bin/graph.cgi?hostname=proxy;plugin=interface;type=if_octets;type_instance=ppp0"
        ;;
    *)
        URL=""  # Default URL - TODO get dummy picture
        ;;
esac

# Iptv plugin listens this port
PORT=${2}

# Stream temporary files
IMAGE=/tmp/image.png
LOG=/dev/null

{
# Using wget because ffmpeg cannot handle http/1.1 "Transfer-Encoding: chunked"
wget -q -O "${IMAGE}" "${URL}"

# Build stream from audiodump with cycle image as video
# PID 0x100/256 = Video 0x101/257 = Audio
exec ffmpeg -v 10 \
  -analyzeduration 0 \
  -loop_input \
  -i "${IMAGE}" \
  -f mpegts -r 25 -vcodec mpeg2video -b 4000k -s 664x540 -padleft 20 -padright 20 -padtop 16 -padbottom 20 \
  -an \
  "udp://127.0.0.1:${PORT}?pkt_size=16356"
} > ${LOG} 2>&1
