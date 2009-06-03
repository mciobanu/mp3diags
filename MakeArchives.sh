#!/bin/bash

#read

function initialize
{
    echo Initializing ...
    rm -f -r package/out
    mkdir -p package/out/Ubuntu
    mkdir -p package/out/Rpm

    Ver=`pwd | sed -e 's%/.*/%%' -e 's% .*%%'`
    Ver=`cat Release.txt`.$Ver

    echo Version: $Ver

    cat package/Rpm/MP3Diags.spec | sed "s+%define version .*$+%define version $Ver+" > package/out/Rpm/MP3Diags.spec
    cat changelogRpm.txt >> package/out/Rpm/MP3Diags.spec

    cat package/Ubuntu/debian.changelog | sed "s%QQQVERQQQ%$Ver%g" > package/out/Ubuntu/debian.changelog
    cat changelogDeb.txt >> package/out/Ubuntu/debian.changelog
    cp -p package/Ubuntu/debian.control package/out/Ubuntu
    cp -p package/Ubuntu/debian.rules package/out/Ubuntu
    cat package/Ubuntu/MP3Diags.dsc | sed "s%QQQVERQQQ%$Ver%g" > package/out/Ubuntu/MP3Diags.dsc
}


function createLinuxSrc
{
    echo Creating Linux source
    DestDir=MP3Diags-$Ver
    LongDestDir=package/out/$DestDir
    rm -f -r $LongDestDir
    mkdir -p $LongDestDir

    cp -pr desktop $LongDestDir
    cp -pr src $LongDestDir
    rm -f -r $LongDestDir/src/debug
    rm -f -r $LongDestDir/src/release
    cp -p COPYING $LongDestDir
    cp -p Install.sh $LongDestDir
    #cp mp3diags.kdevelop $LongDestDir
    cat mp3diags.kdevelop | grep -v "cwd" | grep -v "home" > $LongDestDir/mp3diags.kdevelop
    cp -p mp3diags.pro $LongDestDir
    cp -p Uninstall.sh $LongDestDir

    echo const char* APP_VER '("'$Ver'");'> $LongDestDir/src/Version.cpp
    echo >> $LongDestDir/src/Version.cpp

    for i in $( ls src/licences | sed 's%.*/%%' ); do
        cp -p src/licences/$i $LongDestDir/license.$i
    done

    cd package/out
    tar czf $DestDir.tar.gz $DestDir
    cd ../..

    #rm -f -r $LongDestDir
}


function createWindowsSrc
{
    echo Creating Windows source
    DestDir=MP3DiagsWndSrc-$Ver
    LongDestDir=package/out/$DestDir
    rm -f -r $LongDestDir
    mkdir -p $LongDestDir

    #cp -pr desktop $LongDestDir
    cp -pr src $LongDestDir
    rm -f -r $LongDestDir/src/debug
    rm -f -r $LongDestDir/src/release
    cp -p COPYING $LongDestDir
    #cp -p Install.sh $LongDestDir ttt0 install
    #cp mp3diags.kdevelop $LongDestDir
    #cat mp3diags.kdevelop | grep -v "cwd" | grep -v "home" > $LongDestDir/mp3diags.kdevelop
    #cp -p mp3diags.pro $LongDestDir
    #cp -p Uninstall.sh $LongDestDir
    cp -p Windows/SVGs/* $LongDestDir/src/images

    echo const char* APP_VER '("'$Ver'");'> $LongDestDir/src/Version.cpp
    echo >> $LongDestDir/src/Version.cpp

    for i in $( ls src/licences | sed 's%.*/%%' ); do
        cp -p src/licences/$i $LongDestDir/license.$i
    done

    cd package/out
    #tar czf $DestDir.tar.gz $DestDir
    zip -r -9 $DestDir.zip $DestDir > /dev/null
    cd ../..

    #rm -f -r $LongDestDir
}


#function updateDwnldLinks
#{
#    cat doc/html/010_getting_the_program.html | sed "s%QQQVERQQQ%$Ver%g" > doc/html/010_getting_the_program.html
#}


function fixVersion
{
    cat $1 | sed "s%QQQVERQQQ%$Ver%g" > QQTmpQQ
    rm $1
    mv QQTmpQQ $1
}


function createNoCountDoc
{
    echo Creating non-counted documentation
    DestDir=MP3DiagsNoCountDoc-$Ver
    LongDestDir=package/out/$DestDir
    rm -f -r $LongDestDir
    mkdir -p $LongDestDir

    cp -pr doc/html/*.html $LongDestDir
    fixVersion $LongDestDir/010_getting_the_program.html
    cp -pr doc/html/*.css $LongDestDir
    cp -pr doc/html/*.png $LongDestDir
    cp -pr doc/html/*.ico $LongDestDir
    cp -p COPYING $LongDestDir

    #rm $LongDestDir/010a_getting_the_program.html

    cd package/out
    tar czf $DestDir.tar.gz $DestDir
    cd ../..

    #rm -f -r $LongDestDir
}


function createDoc
{
    echo Creating documentation
    DestDir=MP3DiagsDoc-$Ver
    LongDestDir=package/out/$DestDir
    rm -f -r $LongDestDir
    mkdir -p $LongDestDir

    #cp -pr doc/html/*.html $LongDestDir
    for i in $( ls doc/html/*.html | sed 's%.*/%%' ); do
        cat doc/html/$i | sed 's%QQQStatCounterQQQ% Start of StatCounter Code --> <script type="text/javascript"> var sc_project=4765268; var sc_invisible=1; var sc_partition=54; var sc_click_stat=1; var sc_security="b8120652"; </script> <script type="text/javascript" src="http://www.statcounter.com/counter/counter.js"></script> <noscript> <div class="statcounter"> <a title="website statistics" href="http://www.statcounter.com/" target="_blank"> <img class="statcounter" src="http://c.statcounter.com/4765268/0/b8120652/1/" alt="website statistics" > </a> </div> </noscript> <!-- End of StatCounter Code %' > $LongDestDir/$i
    done
    fixVersion $LongDestDir/010_getting_the_program.html

    cp -pr doc/html/*.css $LongDestDir
    cp -pr doc/html/*.png $LongDestDir
    cp -pr doc/html/*.ico $LongDestDir
    cp -p COPYING $LongDestDir

    #rm $LongDestDir/010a_getting_the_program.html

    cd package/out
    tar czf $DestDir.tar.gz $DestDir
    cd ../..

    #rm -f -r $LongDestDir
}

#pwd > /home/ciobi/cpp/Mp3Utils/MP3Diags/d

initialize
createLinuxSrc
createWindowsSrc
#updateDwnldLinks
createNoCountDoc
createDoc

#FileName=`find . -maxdepth 1 -mindepth 1 -type d | sed s#./##`
