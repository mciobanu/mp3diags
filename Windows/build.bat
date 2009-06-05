PATH=D:\Qt\2009.02\qt\bin;D:\Qt\2009.02\bin;D:\Qt\2009.02\mingw\bin;C:\WINDOWS\System32

cd src
qmake Mp3DiagsWindows.pro
mingw32-make.exe -w release

cd ..
mkdir bin
copy src\release\Mp3DiagsWindows.exe bin
