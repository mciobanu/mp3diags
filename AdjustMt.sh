#!/bin/bash
#
# corrects src/src.pro, so it uses the right serialization library

rm -f -r tstMt
mkdir tstMt
echo "int main() {}" > tstMt/a.cpp
g++ tstMt/a.cpp -lboost_serialization-mt -o tstMt/a.out 2> /dev/null
noMt=$?
#echo $noMt
rm -f -r tstMt

#noMt=0 #ttt remove

if [ $noMt -eq 1 ] ; then
    cat src/src.pro | grep 'lboost_serialization-mt$' > /dev/null
    if [ $? -eq 0 ] ; then # we don't want to change a file that was already changed
        cat src/src.pro | sed 's%lboost_serialization-mt%lboost_serialization%' > src/src.pro1
        mv -f src/src.pro1 src/src.pro
        echo "removed -mt suffix"
    else
        echo "-mt library doesn't exist, but src.pro has been modified already"
    fi
else
    echo "-mt library exists; nothing to do"
fi

