#!/bin/sh

URL=""

if [ -z "${URL}" ]; then
    logger "$0: error: URL not defined!"
    exit 1;
fi

if [ $# -ne 1 ]; then
    logger "$0: error: Invalid parameter count '$#' $*"
    exit 1;
fi

exec vlc "${URL}" --sout "#transcode{vcodec=mp2v,acodec=mpga,vb=800,ab=192}:standard{access=udp,mux=ts,dst=127.0.0.1:${1}}" --intf dummy
