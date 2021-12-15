#!/usr/bin/with-contenv bashio

DIR=$(bashio::config 'input_dir')

inotifyd /root/processNewFile.sh ${DIR}:n 

