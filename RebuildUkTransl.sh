#!/bin/bash

rm src/translations/mp3diags_en_UK.ts
#lupdate src/src.pro 2>&1 | grep -v '^/usr/'
lupdate -no-obsolete src/src.pro 2>&1 | grep -v '^/usr/'
echo -e '\n\n'
src/translations/autoTransl/MixedCase src/translations/mp3diags_en_UK.ts
mv -f /tmp/MiXeDcAsE src/translations/mp3diags_en_UK.ts
lrelease src/src.pro 2>&1 | grep -v '^/usr/'
cp src/translations/*.qm ../mp3diags-build-desktop/bin
