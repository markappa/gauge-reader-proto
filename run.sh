#!/usr/bin/with-contenv bashio

DIR=$(bashio::config 'input_dir')

#export OPENCV_LOG_LEVEL=e

inotifyd /root/processNewFile.sh ${DIR}:n 

