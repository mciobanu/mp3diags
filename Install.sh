#!/bin/bash
#
# Tested on several systems only
# ttt1 Quite likely this needs changes to work with other distros and / or versions

bash ./AdjustMt.sh
#exit 1

QMake=qmake

if [ -f /etc/fedora-release ] ; then
    QMake=qmake-qt4
fi

$QMake
if [ $? -ne 0 ] ; then exit 1 ; fi

make
if [ $? -ne 0 ] ; then exit 1 ; fi

strip bin/MP3Diags

sudo cp bin/MP3Diags /usr/local/bin

