#!/bin/bash
#


if [[ "$#" != "0" ]]; then
    echo "Usage: `basename $0`"
    echo ""
    exit
fi

LRelease=lrelease

if [ -f /etc/fedora-release ] ; then
    LRelease=lrelease-qt4
fi


#lupdate mp3diags.pro
#lrelease src/src.pro
$LRelease src/translations/mp3diags_*.ts

if [[ "$?" != "0" ]] ; then
    echo -e "\nThere was an error trying to build the translations. This shouldn't impact other parts of the build process.\n" >&2
fi
