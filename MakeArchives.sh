#!/bin/bash

#read

function fixVersion
{
    cat $1 | sed -e "s%QQQVERQQQ%$Ver%g" -e "s%QQQBRANCH_SLQQQ%$BranchSlash%g" -e "s%QQQBRANCH_DQQQ%$BranchDash%g" > QQTmpQQ
    origDate=`stat --printf '%Y' $1`
    touch -d @"$origDate" QQTmpQQ
    rm $1
    mv QQTmpQQ $1
}


function initialize
{
    echo Initializing ...
    Ver=`cat Release.txt`
    BranchSlash=`cat branch.txt`
    BranchDash=`echo "$BranchSlash" | sed 's#/#-#'`
    BranchSpace=`echo "$BranchSlash" | sed -e 's#/# #' -e 's# u# U#'`
    BranchComma=`echo "$BranchSlash" | sed -e 's#/#,#' -e 's#,#, #'`

    echo Version: $Ver

    rm -f -r package/out

    head -n 1 changelogDeb.txt | grep $Ver > /dev/null
    if [ $? -ne 0 ] ; then echo "invalid version in deb" ; exit 1 ; fi

    head -n 1 changelogRpm.txt | grep $Ver > /dev/null
    if [ $? -ne 0 ] ; then echo "invalid version in rpm" ; exit 1 ; fi

    head -n 3 changelog.txt | grep $Ver > /dev/null
    if [ $? -ne 0 ] ; then echo "invalid version in changelog" ; exit 1 ; fi

    mkdir -p package/out/deb
    mkdir -p package/out/rpm

    cat package/rpm/MP3Diags.spec | sed -e "s+%define version .*$+%define version $Ver+" -e "s+%define branch .*+%define branch $BranchDash+" -e 's+%define branch $+%define branch %{nil}+' > package/out/rpm/MP3Diags$BranchDash.spec
    cat changelogRpm.txt >> package/out/rpm/MP3Diags$BranchDash.spec
    cat package/rpm/MP3Diags-Mandriva_2009.1.spec | sed -e "s+%define version .*$+%define version $Ver+" -e "s+%define branch .*+%define branch $BranchDash+" -e 's+%define branch $+%define branch %{nil}+' > package/out/rpm/MP3Diags-Mandriva_2009.1$BranchDash.spec
    cat changelogRpm.txt >> package/out/rpm/MP3Diags-Mandriva_2009.1$BranchDash.spec

    cat changelogDeb.txt | sed "s#^mp3diags#mp3diags$BranchDash#" > package/out/deb/debian.changelog
    cat package/deb/debian.control | sed "s#mp3diags#mp3diags$BranchDash#" > package/out/deb/debian.control
    cat package/deb/debian.rules | sed -e "s#bin/MP3Diags#bin/MP3Diags$BranchDash#" -e "s#MP3Diags.desktop#MP3Diags$BranchDash.desktop#" -e "s#debian/mp3diags#debian/mp3diags$BranchDash#" -e "s%MP3Diags.png$%MP3Diags$BranchDash.png%" -e "s%MP3Diags\([0-9][0-9]\)%MP3Diags$BranchDash\1%" -e "s#mp3diags/translations#mp3diags$BranchDash/translations#" > package/out/deb/debian.rules
    cat package/deb/MP3Diags.dsc | sed -e "s%mp3diags%mp3diags$BranchDash%" -e "s%MP3Diags%MP3Diags$BranchDash%" > package/out/deb/MP3Diags$BranchDash.dsc
    fixVersion package/out/deb/MP3Diags$BranchDash.dsc
}


function createPad
{
    echo Creating pad
    LongDestDir=package/out
    year=`date +%Y`
    month=`date +%m`
    day=`date +%d`
    cat package/pad_file.xml | sed "s#<Program_Version>QQQ</Program_Version>#<Program_Version>$Ver</Program_Version>#" | sed "s#<Program_Release_Month>QQQ</Program_Release_Month>#<Program_Release_Month>$month</Program_Release_Month>#"  | sed "s#<Program_Release_Day>QQQ</Program_Release_Day>#<Program_Release_Day>$day</Program_Release_Day>#" | sed "s#<Program_Release_Year>QQQ</Program_Release_Year>#<Program_Release_Year>$year</Program_Release_Year>#" > $LongDestDir/pad_file.xml
}


function createSrc
{
    echo Creating source
    DestDir=MP3Diags$BranchDash-$Ver
    LongDestDir=package/out/$DestDir
    rm -f -r $LongDestDir
    mkdir -p $LongDestDir

    mkdir $LongDestDir/desktop
    for i in `ls desktop/*.png` ; do
        cp -p $i $LongDestDir/$i
    done
    cat desktop/MP3Diags.desktop | sed -e "s#MP3Diags#MP3Diags$BranchDash#" -e "s#MP3 Diags#MP3 Diags$BranchSlash#" > $LongDestDir/desktop/MP3Diags$BranchDash.desktop
    cp -pr src $LongDestDir
    rm -rf $LongDestDir/src/translations/autoTransl
    fixVersion $LongDestDir/src/Helpers.cpp
    cp -p branch.txt $LongDestDir

    rm -f -r $LongDestDir/src/debug
    rm -f -r $LongDestDir/src/release
    rm -f -r $LongDestDir/src/.svn
    rm -f -r $LongDestDir/src/images/.svn
    rm -f -r $LongDestDir/src/licences/.svn
    cp -p COPYING $LongDestDir
    cp -p Install.sh $LongDestDir
    cp -p changelog.txt $LongDestDir
    cp -p mp3diags.pro $LongDestDir
    cp -p Uninstall.sh $LongDestDir
    cp -p AdjustMt.sh $LongDestDir
    cp -p Build.sh $LongDestDir
    cp -p CMakeLists.txt $LongDestDir
    cp -p CMake-VS2008-Win32.cmd $LongDestDir
    cat BuildMp3Diags.hta | sed -e "s#MP3DiagsWindows#MP3DiagsWindows$BranchDash#g" > $LongDestDir/BuildMp3Diags.hta
    cp -p README.TXT $LongDestDir
    cp package/out/pad_file.xml $LongDestDir
    cat MP3DiagsCLI.cmd | sed -e "s#MP3DiagsWindows#MP3DiagsWindows$BranchDash#g" > $LongDestDir/MP3DiagsCLI$BranchDash.cmd

    cat src/Version.cpp | sed -e "s#- custom build#$Ver#" > $LongDestDir/src/Version.cpp

    for i in $( ls src/licences | sed 's%.*/%%' ); do
        cp -p src/licences/$i $LongDestDir/license.$i
    done

    mkdir -p $LongDestDir/package/rpm
    cp -p package/out/rpm/* $LongDestDir/package/rpm
    mkdir -p $LongDestDir/package/deb
    cp -p package/out/deb/* $LongDestDir/package/deb
    rm -f -r $LongDestDir/desktop/.svn

    cp -p BuildBz2.sh $LongDestDir
    fixVersion $LongDestDir/BuildBz2.sh
    chmod a+x $LongDestDir/BuildBz2.sh

    cp -p MakeTranslations.sh $LongDestDir

    cd package/out
    tar czf $DestDir.tar.gz $DestDir -H gnu
    cd ../..

    #rm -f -r $LongDestDir
}

function fixTitleBranch
{
    cat $1 | sed -e "s#MP3, diags#MP3, diags$BranchComma#" -e "s#documentation for MP3 Diags#documentation for MP3 Diags$BranchSpace#" -e "s#<title>MP3 Diags#<title>MP3 Diags$BranchSpace#" > $1.tmp
    mv -f $1.tmp $1
}


#function updateDwnldLinks
#{
#    cat doc/html/010_getting_the_program.html | sed "s%QQQVERQQQ%$Ver%g" > doc/html/010_getting_the_program.html
#}


function createDoc
{
    echo Creating non-counted documentation
    DestDir=MP3DiagsDoc$BranchDash-$Ver
    LongDestDir=package/out/$DestDir
    rm -f -r $LongDestDir
    mkdir -p $LongDestDir

    #cp -pr doc/html/*.html $LongDestDir
    for i in $( ls doc/html/*.html | sed 's%.*/%%' ); do
        origDate=`stat --printf '%Y' doc/html/$i`
        cat doc/html/$i | sed 's# onClick=\"javascript: pageTracker[^\"]*\"##g' > $LongDestDir/$i
        fixTitleBranch $LongDestDir/$i
        touch -d @"$origDate" $LongDestDir/$i
        fixVersion $LongDestDir/$i
    done
    package/AddChangeLog.py $LongDestDir

    cp -pr doc/html/*.css $LongDestDir
    cp -pr doc/html/*.png $LongDestDir
    cp -pr doc/html/*.jpg $LongDestDir
    cp -pr doc/html/*.ico $LongDestDir
    cp -p COPYING $LongDestDir
    mv -f $LongDestDir/010_getting_the_program_local.html $LongDestDir/010_getting_the_program.html

    #rm $LongDestDir/010a_getting_the_program.html

    cd package/out
    tar czf $DestDir.tar.gz $DestDir -H gnu
    cd ../..

    #rm -f -r $LongDestDir
}


function createClicknetDoc
{
    echo Creating Clicknet documentation
    DestDir=MP3DiagsClicknetDoc$BranchDash-$Ver
    LongDestDir=package/out/$DestDir
    rm -f -r $LongDestDir
    mkdir -p $LongDestDir

    #cp -pr doc/html/*.html $LongDestDir
    for i in $( ls doc/html/*.html | sed 's%.*/%%' ); do
        origDate=`stat --printf '%Y' doc/html/$i`
        cat doc/html/$i | sed 's%QQQStatCounterQQQ% Start of StatCounter Code --> <script type="text/javascript"> var sc_project=4841840; var sc_invisible=1; var sc_partition=56; var sc_click_stat=1; var sc_security="644c2333"; </script> <script type="text/javascript" src="http://www.statcounter.com/counter/counter.js"></script><noscript><div class="statcounter"><a title="free hit counter" href="http://www.statcounter.com/free_hit_counter.html" target="_blank"><img class="statcounter" src="http://c.statcounter.com/4841840/0/644c2333/1/" alt="free hit counter" ></a></div></noscript><!-- End of StatCounter Code %' | sed 's# onClick=\"javascript: pageTracker[^\"]*\"##g' > $LongDestDir/$i
        fixTitleBranch $LongDestDir/$i
        touch -d @"$origDate" $LongDestDir/$i
        fixVersion $LongDestDir/$i
    done
    package/AddChangeLog.py $LongDestDir

    cp -pr doc/html/*.css $LongDestDir
    cp -pr doc/html/*.png $LongDestDir
    cp -pr doc/html/*.jpg $LongDestDir
    cp -pr doc/html/*.ico $LongDestDir
    cp -p COPYING $LongDestDir
    rm $LongDestDir/010_getting_the_program_local.html

    #rm $LongDestDir/010a_getting_the_program.html

    cd package/out
    #tar czf $DestDir.tar.gz $DestDir
    cp -p MP3Diags$BranchDash-$Ver.tar.gz $DestDir
    cd ../..

    #rm -f -r $LongDestDir
}


#<!-- Piwik --><script type="text/javascript">var pkBaseURL = (("https:" == document.location.protocol) ? "https://sourceforge.net/apps/piwik/mp3diags/" : "http://sourceforge.net/apps/piwik/mp3diags/");document.write(unescape("%3Cscript src='" + pkBaseURL + "piwik.js' type='text/javascript'%3E%3C/script%3E"));</script><script type="text/javascript">piwik_action_name = '';piwik_idsite = 1;piwik_url = pkBaseURL + "piwik.php";piwik_log(piwik_action_name, piwik_idsite, piwik_url);</script><object><noscript><p><img src="http://sourceforge.net/apps/piwik/mp3diags/piwik.php?idsite=1" alt="piwik"/></p></noscript></object><!-- End Piwik Tag -->


#<script type="text/javascript">
#var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www.");
#document.write(unescape("%3Cscript src='" + gaJsHost + "google-analytics.com/ga.js' type='text/javascript'%3E%3C/script%3E"));
#</script>
#<script type="text/javascript">
#try {
#var pageTracker = _gat._getTracker("UA-11047979-1");
#pageTracker._trackPageview();
#} catch(err) {}</script>


#<!-- GoogleAn --> <script type="text/javascript"> var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www."); document.write(unescape("%3Cscript src='" + gaJsHost + "google-analytics.com/ga.js' type='text/javascript'%3E%3C/script%3E")); </script> <script type="text/javascript"> try { var pageTracker = _gat._getTracker("UA-11047979-1"); pageTracker._trackPageview(); } catch(err) {}</script> <!-- GoogleAn -->

#<!-- GoogleAn --> <script type="text/javascript"> var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www."); document.write(unescape("%3Cscript src='\''" + gaJsHost + "google-analytics.com/ga.js'\'' type='\''text/javascript'\''%3E%3C/script%3E")); </script> <script type="text/javascript"> try { var pageTracker = _gat._getTracker("UA-11047979-1"); pageTracker._trackPageview(); } catch(err) {}</script> <!-- GoogleAn -->


#<script type="text/javascript"> var gaJsHost = (("https:" == document.location.protocol) ? "https://ssl." : "http://www."); document.write(unescape("%3Cscript src='\''" + gaJsHost + "google-analytics.com/ga.js'\'' type='\''text/javascript'\''%3E%3C/script%3E")); </script> <script type="text/javascript"> try { var pageTracker = _gat._getTracker("UA-11047979-1"); pageTracker._trackPageview(); } catch(err) {}</script>

#<script type="text/javascript"> var _gaq = _gaq || []; _gaq.push(['_setAccount', 'UA-11047979-1']); _gaq.push(['_trackPageview']); (function() { var ga = document.createElement('script'); ga.type = 'text/javascript'; ga.async = true; ga.src = ('https:' == document.location.protocol ? 'https://' : 'http://') + 'stats.g.doubleclick.net/dc.js'; var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(ga, s); })(); </script>

#<script type="text/javascript"> var _gaq = _gaq || []; _gaq.push(['\''_setAccount'\'', '\''UA-11047979-1'\'']); _gaq.push(['\''_trackPageview'\'']); (function() { var ga = document.createElement('\''script'\''); ga.type = '\''text/javascript'\''; ga.async = true; ga.src = ('\''https:'\'' == document.location.protocol ? '\''https://'\'' : '\''http://'\'') + '\''stats.g.doubleclick.net/dc.js'\''; var s = document.getElementsByTagName('\''script'\'')[0]; s.parentNode.insertBefore(ga, s); })(); </script>

#<script> (function(i,s,o,g,r,a,m){i['\''GoogleAnalyticsObject'\'']=r;i[r]=i[r]||function(){ (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o), m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m) })(window,document,'\''script'\'','\''//www.google-analytics.com/analytics.js'\'','\''ga'\''); ga('\''create'\'', '\''UA-11047979-1'\'', '\''sourceforge.net'\''); ga('\''send'\'', '\''pageview'\''); </script>

function createSfDoc
{
    echo Creating SF documentation
    DestDir=MP3DiagsSfDoc$BranchDash-$Ver
    LongDestDir=package/out/$DestDir
    rm -f -r $LongDestDir
    mkdir -p $LongDestDir

    #cp -pr doc/html/*.html $LongDestDir
    for i in $( ls doc/html/*.html | sed 's%.*/%%' ); do
        #cat doc/html/$i | sed 's#QQQStatCounterQQQ# Start of StatCounter Code --> <script type="text/javascript"> var sc_project=4765268; var sc_invisible=1; var sc_partition=54; var sc_click_stat=1; var sc_security="b8120652"; </script> <script type="text/javascript" src="http://www.statcounter.com/counter/counter.js"></script> <noscript> <div class="statcounter"> <a title="website statistics" href="http://www.statcounter.com/" target="_blank"> <img class="statcounter" src="http://c.statcounter.com/4765268/0/b8120652/1/" alt="website statistics" > </a> </div> </noscript> <!-- End of StatCounter Code -->          <!-- Piwik --><script type="text/javascript">var pkBaseURL = (("https:" == document.location.protocol) ? "https://sourceforge.net/apps/piwik/mp3diags/" : "http://sourceforge.net/apps/piwik/mp3diags/");document.write(unescape("%3Cscript src='\''" + pkBaseURL + "piwik.js'\'' type='\''text/javascript'\''%3E%3C/script%3E"));</script><script type="text/javascript">piwik_action_name = '\'''\'';piwik_idsite = 1;piwik_url = pkBaseURL + "piwik.php";piwik_log(piwik_action_name, piwik_idsite, piwik_url);</script><object><noscript><p><img src="http://sourceforge.net/apps/piwik/mp3diags/piwik.php?idsite=1" alt="piwik"/></p></noscript></object><!-- End Piwik Tag #' | sed 's#<!-- sf_hosting -->#<td border="0" class="HeaderText HeaderPadRight RightAlign"><a href="http://sourceforge.net/projects/mp3diags"><img border="0" align=middle src="http://sflogo.sourceforge.net/sflogo.php?group_id=260878\&amp;type=12" width="120" height="30" alt="Get MP3 Diags at SourceForge.net. Fast, secure and Free Open Source software downloads" /></a></td>#' | sed 's#<!-- add_this_conf -->#<script type="text/javascript"> var addthis_config = { username: "ciobi" } </script><script type="text/javascript" src="http://s7.addthis.com/js/250/addthis_widget.js"></script>#' | sed 's#<!-- add_this_link -->#<td border="0" class="HeaderText HeaderPadRight"><a href="http://www.addthis.com/bookmark.php?v=250" class="addthis_button" addthis:url="http://mp3diags.sourceforge.net/" addthis:title="MP3 Diags"> <img src="http://s7.addthis.com/static/btn/lg-share-en.gif" width="125" height="16" border="0" alt="Share" align="absmiddle" /> </a></td>#' > $LongDestDir/$i

    #<img src="http://s7.addthis.com/static/btn/lg-share-en.gif" width="125" height="16" border="0" alt="Share" align="middle" />

        origDate=`stat --printf '%Y' doc/html/$i`

        cat doc/html/$i | sed 's#QQQStatCounterQQQ# Start of StatCounter Code --> <script type="text/javascript"> var sc_project=4765268; var sc_invisible=1; var sc_partition=54; var sc_click_stat=1; var sc_security="b8120652"; </script> <script type="text/javascript" src="http://www.statcounter.com/counter/counter.js"></script> <noscript> <div class="statcounter"> <a title="website statistics" href="http://www.statcounter.com/" target="_blank"> <img class="statcounter" src="http://c.statcounter.com/4765268/0/b8120652/1/" alt="website statistics" > </a> </div> </noscript> <!-- End of StatCounter Code -->          <!-- Piwik --><script type="text/javascript">var pkBaseURL = (("https:" == document.location.protocol) ? "https://sourceforge.net/apps/piwik/mp3diags/" : "http://sourceforge.net/apps/piwik/mp3diags/");document.write(unescape("%3Cscript src='\''" + pkBaseURL + "piwik.js'\'' type='\''text/javascript'\''%3E%3C/script%3E"));</script><script type="text/javascript">piwik_action_name = '\'''\'';piwik_idsite = 1;piwik_url = pkBaseURL + "piwik.php";piwik_log(piwik_action_name, piwik_idsite, piwik_url);</script><object><noscript><p><img src="http://sourceforge.net/apps/piwik/mp3diags/piwik.php?idsite=1" alt="piwik"/></p></noscript></object><!-- End Piwik Tag #' | sed 's#<!-- sf_hosting -->#<td border="0" class="HeaderText HeaderPadRight RightAlign"><a href="http://sourceforge.net/projects/mp3diags"><img border="0" align=middle src="http://sflogo.sourceforge.net/sflogo.php?group_id=260878\&amp;type=12" width="120" height="30" alt="Get MP3 Diags at SourceForge.net. Fast, secure and Free Open Source software downloads" /></a></td>#' | sed 's#<!-- add_this_conf -->#<script type="text/javascript"> var addthis_config = { username: "ciobi", data_use_flash: false } </script><script type="text/javascript" src="http://s7.addthis.com/js/250/addthis_widget.js"></script>        <!-- GoogleAn --> <script> (function(i,s,o,g,r,a,m){i['\''GoogleAnalyticsObject'\'']=r;i[r]=i[r]||function(){ (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o), m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m) })(window,document,'\''script'\'','\''//www.google-analytics.com/analytics.js'\'','\''ga'\''); ga('\''create'\'', '\''UA-11047979-1'\'', '\''sourceforge.net'\''); ga('\''send'\'', '\''pageview'\''); </script> <!-- GoogleAn -->#' | sed 's#<!-- add_this_link -->#<a href="http://www.addthis.com/bookmark.php?v=250" class="addthis_button" addthis:url="http://mp3diags.sourceforge.net/" addthis:title="MP3 Diags"><img src="share.gif" border="0" alt="Share" align="middle" /></a>#' | sed "s#pageTracker._trackPageview.'#pageTracker._trackPageview\\('$BranchSlash#" > $LongDestDir/$i
        touch -d @"$origDate" $LongDestDir/$i
        fixTitleBranch $LongDestDir/$i
        fixVersion $LongDestDir/$i
    done
    package/AddChangeLog.py $LongDestDir

    cp -pr doc/html/*.css $LongDestDir
    cp -pr doc/html/*.png $LongDestDir
    cp -pr doc/html/*.jpg $LongDestDir
    cp -pr doc/html/*.ico $LongDestDir
    cp -pr doc/html/*.gif $LongDestDir
    cp -p COPYING $LongDestDir
    cp package/out/pad_file.xml $LongDestDir
    rm $LongDestDir/010_getting_the_program_local.html

    echo $Ver > $LongDestDir/version.txt

    #rm $LongDestDir/010a_getting_the_program.html

    cd package/out
    #tar czf $DestDir.tar.gz $DestDir
    cd ../..

    #rm -f -r $LongDestDir
}

function createPackagerSrc
{
    echo Creating Source+Doc bundle
    cd package/out
    mkdir MP3Diags$BranchDash-$Ver/doc
    cp -p MP3DiagsDoc$BranchDash-$Ver/* MP3Diags$BranchDash-$Ver/doc
    tar czf MP3Diags"$BranchDash"_Src+Doc-$Ver.tar.gz MP3Diags$BranchDash-$Ver -H gnu
    mv MP3Diags"$BranchDash"_Src+Doc-$Ver.tar.gz MP3DiagsClicknetDoc$BranchDash-$Ver
    cd ../..
}






#pwd > /home/ciobi/cpp/Mp3Utils/MP3Diags/d

initialize
createPad
createSrc
createDoc
createClicknetDoc
createSfDoc
createPackagerSrc

if [ -f CopyToSf.sh ] ; then
    cp -p CopyToSf.sh package/out
fi

cp -p branch.txt package/out

#FileName=`find . -maxdepth 1 -mindepth 1 -type d | sed s#./##`
