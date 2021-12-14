#!/bin/bash

#apk add opencv 
apk add opencv-dev 
apk add musl-dev 
apk add g++

alias ll="ls -la"

# build cxxopts
apk add git
#apk add cmake
#export CMAKE_CXX_COMPILER=g++
apk add make 
mkdir cxxopts
cd cxxopts
git pull https://github.com/jarro2783/cxxopts master
make .
make install
cd ..

