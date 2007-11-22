#!/bin/sh

if [ $# -ne 2 ]; then
    logger "$0: error: Invalid parameter count '$#' $*"
    exit 1;
fi

# Channels.conf parameter
PARAMETER=${1}

# Iptv plugin listens this port
PORT=${2}

# Default bitrates for stream transcoding
VBITRATE=2400
ABITRATE=320

# There is a way to specify multiple URLs in the same script. The selection is
# then controlled by the extra parameter passed by IPTV plugin to the script
case $PARAMETER in
    1)
	URL=""
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
    exit 1;
fi

# Create unique pids for the stream
let VPID=${PARAMETER}+1
let APID=${PARAMETER}+2
let SPID=${PARAMETER}+3

# Capture program pid for further management in IPTV plugin
vlc "${URL}" --sout "#transcode{vcodec=mp2v,acodec=mpga,vb=${VBITRATE},ab=${ABITRATE}}:standard{access=udp,mux=ts{pid-video=${VPID},pid-audio=${APID},pid-spu=${SPID}},dst=127.0.0.1:${PORT}}" --intf dummy &

PID=${!}

trap 'kill -INT ${PID} 2> /dev/null' INT EXIT QUIT TERM

# Waiting for the given PID to terminate
wait ${PID}

