#!/bin/bash
#

#ttt1 only works for one user, but it's likely that this is the case when it's most needed

BranchSlash=`cat branch.txt`
BranchDash=`echo "$BranchSlash" | sed 's#/#-#'`
exe=MP3Diags$BranchDash

/usr/local/bin/$exe -u
sudo rm /usr/local/bin/$exe
rm ~/.config/Ciobi/$exe.conf

#transl=/usr/local/share/mp3diags"$BranchDash"/translations
share=/usr/local/share/mp3diags"$BranchDash"
sudo rm -rf "$share"

echo
echo "If other users started MP3 Diags as well, they will have to remove their configuration file manually (it\'s ~/.config/Ciobi/Mp3Diags.conf)"
echo "Sessions files (.ini and associated .dat) can only be removed manually"
echo
