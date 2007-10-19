#!/bin/sh

if [ $# -ne 2 ]; then
    logger "$0: error: Invalid parameter count '$#' $*"
    exit 1;
fi

# Channels.conf parameter
PARAMETER=${1}

# Iptv plugin listens this port
PORT=${2}

# Define stream address
URL=""

if [ -z "${URL}" ]; then
    logger "$0: error: URL not defined!"
    exit 1;
fi

# Use 'exec' for capturing script pid
exec vlc "${URL}" --sout "#transcode{vcodec=mp2v,acodec=mpga,vb=800,ab=192}:standard{access=udp,mux=ts,dst=127.0.0.1:${PORT}}" --intf dummy
