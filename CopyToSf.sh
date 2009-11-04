#!/bin/sh
#

echo Kso9di8Yuw3 > zSfPasswd.txt

RSYNC_PASSWORD=Kso9di8Yuw3
export RSYNC_PASSWORD

#rsync -avP -e ssh MP3Diags-*.tar.gz ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-src < zSfPasswd.txt
#rsync -avP -e ssh Mp3DiagsExe-*.zip ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-exe < zSfPasswd.txt
#rsync -avP -e ssh MP3DiagsSetup-*.exe ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-setup < zSfPasswd.txt
#rsync -avP -e ssh MP3DiagsDoc-*.tar.gz ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-doc < zSfPasswd.txt
#rsync -avP -e ssh MP3Diags-*-i686.bz2 ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/i686 < zSfPasswd.txt
#rsync -avP -e ssh MP3Diags-*-x86_64.bz2 ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/x86_64 < zSfPasswd.txt
#

#rsync -avP -e ssh MP3Diags-*.tar.gz ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-src
#rsync -avP -e ssh MP3DiagsExe-*.zip ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-exe
#rsync -avP -e ssh MP3DiagsSetup-*.exe ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-setup
#rsync -avP -e ssh MP3DiagsDoc-*.tar.gz ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-doc
#rsync -avP -e ssh MP3Diags-*-i686.bz2 ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/i686
#rsync -avP -e ssh MP3Diags-*-x86_64.bz2 ciobi07@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/x86_64


#rsync -avP -e ssh MP3Diags-Linux-x86_64-*.bz2 ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-linux-bin/x86_64
#rsync -e ssh MP3DiagsDoc-*.tar.gz ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-doc
#rsync --password-file=zSfPasswd.txt -avP -e ssh MP3Diags-*.tar.gz ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-src

#rsync -avP -e ssh Mp3DiagsExe-*.zip ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-exe < zSfPasswd.txt
#rsync -e ssh MP3DiagsExe-*.zip ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-exe
rsync -e ssh MP3DiagsSetup-*.exe ciobi07,mp3diags@frs.sourceforge.net:/home/frs/project/m/mp/mp3diags/mp3diags-windows-setup


rm zSfPasswd.txt
