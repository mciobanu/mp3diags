#!/bin/sh
#

#ttt1 only works for one user, but it's likely that this is the case when it's most needed

/usr/local/bin/MP3Diags /u
sudo rm /usr/local/bin/MP3Diags
rm ~/.config/Ciobi/Mp3Diags.conf

echo
echo If other users started MP3 Diags as well, they will have to remove their configuration file manually (it\'s ~/.config/Ciobi/Mp3Diags.conf)
echo Sessions files (.ini and associated .dat) can only be removed manually
echo
