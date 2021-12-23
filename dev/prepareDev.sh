#!/bin/bash

#apk add opencv 
apk add opencv-dev 
apk add musl-dev 
apk add g++
apk add vim

alias ll="ls -lah"
alias l="ls"
alias vi="vim"
alias gs="git status"
export PATH=$PATH:.

# build cxxopts
apk add git
apk add cmake
#export CMAKE_CXX_COMPILER=g++
apk add make 
mkdir cxxopts
cd cxxopts
git init
git pull https://github.com/jarro2783/cxxopts master
cmake .
make
make install
cd ..

