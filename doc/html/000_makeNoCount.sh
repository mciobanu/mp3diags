#!/bin/sh
#

if [[ $1 != "" ]]; then
    echo "Usage: `basename $0`"
    echo ""
    exit
fi


for i in $( find . -mindepth 1 -maxdepth 1 -type f -iregex '.*[^~]$' | sed s%./%% ); do
    cp -p $i no_count/$i
done


for i in $( ls *.html ); do
    cat $i | sed "s%<\!-- Start of StatCounter.*%%" > no_count/$i
done


