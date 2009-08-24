PATH=C:\Qt\2009.03\qt\bin;C:\Qt\2009.03\bin;C:\Qt\2009.03\mingw\bin;C:\WINDOWS\System32

cd src
qmake Mp3DiagsWindows.pro
mingw32-make.exe -w release

cd ..
mkdir bin
copy src\release\Mp3DiagsWindows.exe bin
copy changelog.txt bin\changelog.txt
