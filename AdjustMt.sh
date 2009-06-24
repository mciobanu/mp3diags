#!/bin/bash

rm -f -r tstMt
mkdir tstMt
echo "int main() {}" > tstMt/a.cpp
g++ tstMt/a.cpp -lboost_serialization-mt -o tstMt/a.out 2> /dev/null
noMt=$?
#echo $noMt
rm -f -r tmp

#noMt=0 #ttt remove

if [ $noMt -eq 0 ] ; then
    cat src/src.pro | grep 'lboost_serialization$' > /dev/null
    if [ $? -eq 0 ] ; then # we don't want to change a file that was already changed
        cat src/src.pro | sed 's%lboost_serialization%lboost_serialization-mt%' > src/src.pro1
        mv -f src/src.pro1 src/src.pro
    fi
fi

