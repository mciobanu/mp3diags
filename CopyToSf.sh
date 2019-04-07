#!/bin/sh
#

BranchSlash=`cat branch.txt`
BranchDash=`echo "$BranchSlash" | sed 's#/#-#'`

rm -rf mp3diags


#disabled until linux binary available# mkdir -p mp3diags"$BranchSlash"/mp3diags-linux-bin/x86_64
#disabled until linux binary available# cp -p MP3Diags"$BranchDash"-Linux-x86_64-*.tar.bz2 mp3diags"$BranchSlash"/mp3diags-linux-bin/x86_64/MP3Diags"$BranchDash"-Linux-x86_64.tar.bz2 ; cp -p MP3Diags"$BranchDash"-Linux-x86_64-*.tar.bz2 mp3diags"$BranchSlash"/mp3diags-linux-bin/x86_64/

#disabled until linux binary available# mkdir -p mp3diags"$BranchSlash"/mp3diags-linux-bin/i686
#disabled until linux binary available# cp -p MP3Diags"$BranchDash"-Linux-i686-*.tar.bz2 mp3diags"$BranchSlash"/mp3diags-linux-bin/i686/MP3Diags"$BranchDash"-Linux-i686.tar.bz2 ; cp -p MP3Diags"$BranchDash"-Linux-i686-*.tar.bz2 mp3diags"$BranchSlash"/mp3diags-linux-bin/i686/

mkdir -p mp3diags"$BranchSlash"/mp3diags-doc
cp -p MP3DiagsDoc"$BranchDash"-*.tar.gz mp3diags"$BranchSlash"/mp3diags-doc/MP3DiagsDoc"$BranchDash".tar.gz ; cp -p MP3DiagsDoc"$BranchDash"-*.tar.gz mp3diags"$BranchSlash"/mp3diags-doc/

mkdir -p mp3diags"$BranchSlash"/mp3diags-src
cp -p MP3Diags"$BranchDash"-*.tar.gz mp3diags"$BranchSlash"/mp3diags-src/MP3Diags"$BranchDash".tar.gz ; cp -p MP3Diags"$BranchDash"-*.tar.gz mp3diags"$BranchSlash"/mp3diags-src/

mkdir -p mp3diags"$BranchSlash"/mp3diags-windows-exe
cp -p MP3DiagsExe"$BranchDash"-*.zip mp3diags"$BranchSlash"/mp3diags-windows-exe/MP3DiagsExe"$BranchDash".zip ; cp -p MP3DiagsExe"$BranchDash"-*.zip mp3diags"$BranchSlash"/mp3diags-windows-exe/

mkdir -p mp3diags"$BranchSlash"/mp3diags-windows-setup
cp -p MP3DiagsSetup"$BranchDash"-*.exe mp3diags"$BranchSlash"/mp3diags-windows-setup/MP3DiagsSetup"$BranchDash".exe ; cp -p MP3DiagsSetup"$BranchDash"-*.exe mp3diags"$BranchSlash"/mp3diags-windows-setup/


# the point of the "t" files is to update the directories' dates; since there are nested directories,
# at the top level of the "unstable" or of "linux-bin" there's no indication that newer versions are available

# !!! touch creates empty files, which aren't shown at SF, so we need some content
#disabled until linux binary available# date > mp3diags"$BranchSlash"/mp3diags-linux-bin/t
date > mp3diags"$BranchSlash"/t


rsync -avP -e ssh mp3diags/ ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags


# allow change to be detected ttt0 see if long enough
echo 'sleeping 20 seconds ...'
sleep 20

#disabled until linux binary available# rm mp3diags"$BranchSlash"/mp3diags-linux-bin/t
rm mp3diags"$BranchSlash"/t
#disabled until linux binary available# rsync -avP --delete -e ssh mp3diags"$BranchSlash"/mp3diags-linux-bin/ --include=t --exclude='*' ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags"$BranchSlash"/mp3diags-linux-bin
rsync -avP --delete -e ssh mp3diags"$BranchSlash"/ --include=t --exclude='*' ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags"$BranchSlash"



# The next line is enough to copy the documentation
BranchSlash=`cat branch.txt` ; rsync -avP -e ssh MP3DiagsSfDoc*/ ciobi07,mp3diags@web.sourceforge.net:htdocs"$BranchSlash"/

