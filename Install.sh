#!/bin/bash
#
# Tested on several systems only
# ttt1 Quite likely this needs changes to work with other distros and / or versions
#
# by passing the param "QMAKE_CXX=clang" the project will be compiled with clang

bash ./AdjustMt.sh
#exit 1

QMake=qmake

if [ -f /etc/fedora-release ] ; then
    QMake=qmake-qt4
fi

if [[ "$1" != "" ]] ; then
    $QMake "$1"
else
    $QMake
fi

if [ $? -ne 0 ] ; then exit 1 ; fi

make
if [ $? -ne 0 ] ; then exit 1 ; fi

BranchSlash=`cat branch.txt`
BranchDash=`echo "$BranchSlash" | sed 's#/#-#'`
exe=MP3Diags$BranchDash
transl=/usr/local/share/mp3diags"$BranchDash"/translations

./MakeTranslations.sh
cp src/translations/*.qm bin

strip bin/$exe

sudo cp bin/$exe /usr/local/bin
sudo mkdir -p "$transl"
sudo cp bin/*.qm "$transl"
