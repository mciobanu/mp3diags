@echo off
rem this file is overwritten by the NSIS script, using the full dir for the exe

MP3DiagsWindows.exe %* > %TEMP%\Mp3DiagsOut.txt
type %TEMP%\Mp3DiagsOut.txt
del %TEMP%\Mp3DiagsOut.txt
