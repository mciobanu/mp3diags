#!/bin/sh
#
# Tested on several systems only
# ttt1 Quite likely this needs changes to work with other distros and / or versions

QMake=qmake

if [ -f /etc/fedora-release ] ; then
    QMake=qmake-qt4
fi

$QMake
make
sudo cp bin/MP3Diags /usr/local/bin

