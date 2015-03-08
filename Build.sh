#!/bin/bash
#
# Builds MP3 Diags
#
# Tested on several systems only
# ttt1 Quite likely this needs changes to work with other distros and / or versions
#
# by passing the param "QMAKE_CXX=clang" the project will be compiled with clang

./AdjustMt.sh $MP3_DIAGS_STATIC

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

./MakeTranslations.sh
cp src/translations/*.qm bin

BranchSlash=`cat branch.txt`
BranchDash=`echo "$BranchSlash" | sed 's#/#-#'`
MP3DiagsExe=MP3Diags$BranchDash

strip bin/$MP3DiagsExe
