#!/bin/bash
#
# Builds MP3 Diags linking Boost Serialization statically, for creation of "generic" binaries
#
# Tested on several systems only
# ttt1 Quite likely this needs changes to work with other distros and / or versions
#
# by passing the param "QMAKE_CXX=clang" the project will be compiled with clang


bash ./AdjustMt.sh STATIC_SER

#cat src/src.pro | grep 'lboost_serialization' > /dev/null
#if [ $? -eq 0 ] ; then # we don't want to change a file that was already changed
#    cat src/src.pro | sed -e 's%lboost_serialization-mt$%l:libboost_serialization-mt.a%' -e 's%lboost_serialization$%l:libboost_serialization.a%' > src/src.pro1
#    mv -f src/src.pro1 src/src.pro
#    echo "switched to static linking"
#else
#    echo "static linking already set up"
#fi


#exit 1

QMake=qmake

if [ -f /etc/fedora-release ] ; then
    QMake=qmake-qt4
fi

$QMake "$1"
if [ $? -ne 0 ] ; then exit 1 ; fi

make
if [ $? -ne 0 ] ; then exit 1 ; fi

BranchSlash=`cat branch.txt`
BranchDash=`echo "$BranchSlash" | sed 's#/#-#'`
exe=MP3Diags$BranchDash

strip bin/$exe

Cpu=`uname -m`

cd bin
NewName=$exe-Linux-$Cpu-QQQVERQQQ
mv $exe $NewName
tar -c $NewName | bzip2 > $NewName.tar.bz2
mv $NewName $exe
cd ..
