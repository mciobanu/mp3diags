#!/bin/bash
#
# Tested on several systems only
# ttt1 Quite likely this needs changes to work with other distros and / or versions
#
# by passing the param "QMAKE_CXX=clang" the project will be compiled with clang

MP3_DIAGS_STATIC=""
. ./Build.sh

transl=/usr/local/share/mp3diags"$BranchDash"/translations
sudo cp bin/$MP3DiagsExe /usr/local/bin
sudo mkdir -p "$transl"
sudo cp bin/*.qm "$transl"
