#!/bin/bash
#


if [[ "$#" != "0" ]]; then
    echo "Usage: `basename $0`"
    echo ""
    exit
fi

#lupdate mp3diags.pro
lrelease src/src.pro

if [[ "$?" != "0" ]] ; then
    echo -e "\nThere was an error trying to build the translations. This shouldn't impact other parts of the build process.\n" >&2
fi
