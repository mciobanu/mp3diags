#!/bin/sh
#

BranchSlash=`cat branch.txt`
BranchDash=`echo "$BranchSlash" | sed 's#/#-#'`

rm -rf mp3diags


mkdir -p mp3diags"$BranchSlash"/mp3diags-linux-bin/x86_64
cp -p MP3Diags"$BranchDash"-Linux-x86_64-*.tar.bz2 mp3diags"$BranchSlash"/mp3diags-linux-bin/x86_64/MP3Diags"$BranchDash"-Linux-x86_64.tar.bz2 ; cp -p MP3Diags"$BranchDash"-Linux-x86_64-*.tar.bz2 mp3diags"$BranchSlash"/mp3diags-linux-bin/x86_64/

mkdir -p mp3diags"$BranchSlash"/mp3diags-linux-bin/i686
cp -p MP3Diags"$BranchDash"-Linux-i686-*.tar.bz2 mp3diags"$BranchSlash"/mp3diags-linux-bin/i686/MP3Diags"$BranchDash"-Linux-i686.tar.bz2 ; cp -p MP3Diags"$BranchDash"-Linux-i686-*.tar.bz2 mp3diags"$BranchSlash"/mp3diags-linux-bin/i686/

mkdir -p mp3diags"$BranchSlash"/mp3diags-doc
cp -p MP3DiagsDoc"$BranchDash"-*.tar.gz mp3diags"$BranchSlash"/mp3diags-doc/MP3DiagsDoc"$BranchDash".tar.gz ; cp -p MP3DiagsDoc"$BranchDash"-*.tar.gz mp3diags"$BranchSlash"/mp3diags-doc/

mkdir -p mp3diags"$BranchSlash"/mp3diags-src
cp -p MP3Diags"$BranchDash"-*.tar.gz mp3diags"$BranchSlash"/mp3diags-src/MP3Diags"$BranchDash".tar.gz ; cp -p MP3Diags"$BranchDash"-*.tar.gz mp3diags"$BranchSlash"/mp3diags-src/

mkdir -p mp3diags"$BranchSlash"/mp3diags-windows-exe
cp -p MP3DiagsExe"$BranchDash"-*.zip mp3diags"$BranchSlash"/mp3diags-windows-exe/MP3DiagsExe"$BranchDash".zip ; cp -p MP3DiagsExe"$BranchDash"-*.zip mp3diags"$BranchSlash"/mp3diags-windows-exe/

mkdir -p mp3diags"$BranchSlash"/mp3diags-windows-setup
cp -p MP3DiagsSetup"$BranchDash"-*.exe mp3diags"$BranchSlash"/mp3diags-windows-setup/MP3DiagsSetup"$BranchDash".exe ; cp -p MP3DiagsSetup"$BranchDash"-*.exe mp3diags"$BranchSlash"/mp3diags-windows-setup/


rsync -avP -e ssh mp3diags/* ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags



# echo Kso9di8Yuw3 > zSfPasswd.txt
#
# RSYNC_PASSWORD=Kso9di8Yuw3
# export RSYNC_PASSWORD
#
# #rsync -avP -e ssh MP3Diags-*.tar.gz ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-src < zSfPasswd.txt
# #rsync -avP -e ssh Mp3DiagsExe-*.zip ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-exe < zSfPasswd.txt
# #rsync -avP -e ssh MP3DiagsSetup-*.exe ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-setup < zSfPasswd.txt
# #rsync -avP -e ssh MP3DiagsDoc-*.tar.gz ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-doc < zSfPasswd.txt
# #rsync -avP -e ssh MP3Diags-*-i686.bz2 ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/i686 < zSfPasswd.txt
# #rsync -avP -e ssh MP3Diags-*-x86_64.bz2 ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/x86_64 < zSfPasswd.txt
# #
#
# #rsync -avP -e ssh MP3Diags-*.tar.gz ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-src
# #rsync -avP -e ssh MP3DiagsExe-*.zip ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-exe
# #rsync -avP -e ssh MP3DiagsSetup-*.exe ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-setup
# #rsync -avP -e ssh MP3DiagsDoc-*.tar.gz ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-doc
# #rsync -avP -e ssh MP3Diags-*-i686.bz2 ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/i686
# #rsync -avP -e ssh MP3Diags-*-x86_64.bz2 ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/x86_64
#
#
# #rsync -avP -e ssh MP3Diags-Linux-x86_64-*.bz2 ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/x86_64
# #rsync -e ssh MP3DiagsDoc-*.tar.gz ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-doc
# #rsync --password-file=zSfPasswd.txt -avP -e ssh MP3Diags-*.tar.gz ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-src
#
# #rsync -avP -e ssh Mp3DiagsExe-*.zip ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-exe < zSfPasswd.txt
# #rsync -e ssh MP3DiagsExe-*.zip ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-exe
# rsync -e ssh MP3DiagsSetup-*.exe ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-setup
#
#
# rm zSfPasswd.txt
#
# rsync -avP -e ssh MP3Diags-*.tar.gz ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-src
#
#
#
# rsync -avP -e ssh MP3Diags-*.tar.gz ciobi07,mp3diags@shell.sourceforge.net
#
#
#
# rsync -avP -e ssh id_rsa.pub ciobi07,mp3diags@frs.sourceforge.net:/home/users/c/ci/ciobi07
#
# rsync -avP -e ssh id_rsa.pub ciobi07,mp3diags@shell.sourceforge.net:/home/users/c/ci/ciobi07
#

# ln -s -f MP3Diags-Linux-x86_64-*.bz2 MP3Diags-Linux-x86_64.bz2 ; rsync -avP -e ssh MP3Diags-Linux-x86_64*.bz2 ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/x86_64
# ln -s -f MP3Diags-Linux-i686-*.bz2 MP3Diags-Linux-i686.bz2 ; rsync -avP -e ssh MP3Diags-Linux-i686*.bz2 ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/i686
# ln -s -f MP3DiagsDoc-*.tar.gz MP3DiagsDoc.tar.gz ; rsync -avP -e ssh MP3DiagsDoc*.tar.gz ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-doc
# ln -s -f MP3Diags-*.tar.gz MP3Diags.tar.gz ; rsync -avP -e ssh MP3Diags-*.tar.gz MP3Diags.tar.gz ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-src
#
# ln -s -f MP3DiagsExe-*.zip MP3DiagsExe.zip ; rsync -avP -e ssh MP3DiagsExe*.zip ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-exe
# ln -s -f MP3DiagsSetup-*.exe MP3DiagsSetup.exe ; rsync -avP -e ssh MP3DiagsSetup*.exe ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-setup

