@echo off
MP3DiagsWindows.exe %* > %TEMP%\Mp3DiagsOut.txt
type %TEMP%\Mp3DiagsOut.txt
del %TEMP%\Mp3DiagsOut.txt
