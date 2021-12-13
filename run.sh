#!/usr/bin/with-contenv bashio

SCREENSHOT_DIR=$(bashio::config 'screenshot_dir')

if [ $SCREENSHOT_DIR = "null" ]
then
SCREENSHOT_DIR="/media/espcam_02"
fi

export OPENCV_LOG_LEVEL=e

inotifyd /root/processNewFile.sh ${SCREENSHOT_DIR}:n 

