#!/bin/bash
#
# Builds MP3 Diags linking Boost libraries statically, if possible, for creation of "generic" binaries
#
# Tested on several systems only
# ttt1 Quite likely this needs changes to work with other distros and / or versions
#
# by passing the param "QMAKE_CXX=clang" the project will be compiled with clang

if [ -d bin ] ; then
    rm bin/*
fi

MP3_DIAGS_STATIC=STATIC_SER
. ./Build.sh

Cpu=`uname -m`

NewName=$MP3DiagsExe-Linux-$Cpu-QQQVERQQQ
mv bin/$MP3DiagsExe bin/$NewName
# force gnu format because some not very old tools have trouble understanding the newer pax format
tar -H gnu -c bin | bzip2 > $NewName.tar.bz2
mv $NewName.tar.bz2 bin
mv bin/$NewName bin/$MP3DiagsExe

