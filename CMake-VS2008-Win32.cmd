@echo off

: Do not modify the caller's environment and enable "!" expansion.
setlocal enabledelayedexpansion

: (Re-)create an empty output directory.
rmdir /s /q VS2008-Win32 2> nul
mkdir VS2008-Win32 2> nul

: Search QTDIR first, then C:\Qt\**, and finally CMAKE_PREFIX_PATH.
for /d %%d in (C:\Qt\*) do set CMAKE_PREFIX_PATH=%%d;!CMAKE_PREFIX_PATH!
set CMAKE_PREFIX_PATH=%QTDIR%;%CMAKE_PREFIX_PATH%

: Generate the project files in the output directory.
pushd VS2008-Win32
cmake -G "Visual Studio 9 2008" ..
if errorlevel 1 pause
popd
