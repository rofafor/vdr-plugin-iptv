#!/bin/sh
#
# iptvstream.sh can be used by the VDR iptv plugin to transcode external
# sources
#
# (C) 2007 Rolf Ahrenberg, Antti Seppälä
#
# iptvstream.sh is free software; you can redistribute it and/or modify
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

if [ $# -ne 2 ]; then
    logger "$0: error: Invalid parameter count '$#' $*"
    exit 1
fi

# Channels.conf parameter
PARAMETER=${1}

# Iptv plugin listens this port
PORT=${2}

# Default settings for stream transcoding
VCODEC=mp2v
VBITRATE=2400
ACODEC=mpga
ABITRATE=320

# There is a way to specify multiple URLs in the same script. The selection is
# then controlled by the extra parameter passed by IPTV plugin to the script
case ${PARAMETER} in
    1)
        URL=""
        WIDTH=720
        HEIGHT=576
        FPS=25
        ;;
    2)
        URL=""
        ;;
    3)
        URL=""
        ;;
    *)
        URL=""  # Default URL
        ;;
esac

if [ -z "${URL}" ]; then
    logger "$0: error: URL not defined!"
    exit 1
fi

# Create transcoding options
TRANSCODE_OPTS="vcodec=${VCODEC},acodec=${ACODEC},vb=${VBITRATE},ab=${ABITRATE}"
if [ -n "${WIDTH}" -a -n "${HEIGHT}" ] ; then
    TRANSCODE_OPTS="${TRANSCODE_OPTS},width=${WIDTH},height=${HEIGHT}"
fi
if [ -n "${FPS}" ] ; then
    TRANSCODE_OPTS="${TRANSCODE_OPTS},fps=${FPS}"
fi

# Create unique pids for the stream
let VPID=${PARAMETER}+1
let APID=${PARAMETER}+2
let SPID=${PARAMETER}+3

# Capture VLC pid for further management in IPTV plugin
vlc "${URL}" --sout "#transcode{${TRANSCODE_OPTS}}:standard{access=udp,mux=ts{pid-video=${VPID},pid-audio=${APID},pid-spu=${SPID}},dst=127.0.0.1:${PORT}}" --intf dummy &

PID=${!}

trap 'kill -INT ${PID} 2> /dev/null' INT EXIT QUIT TERM

# Waiting for the given PID to terminate
wait ${PID}
