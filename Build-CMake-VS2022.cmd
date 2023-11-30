@echo off

rem Creates some dirs, calls cmake, copies some files, so in the end the executable in build\Release\dist
rem should have all its dependencies and be able to run.
rem
rem Requirements:
rem Visual Studio, Qt5, Boost, and ZLib need to be installed.
rem Boost_ROOT and ZLIB_ROOT environment variables should be defined and vcvarsall.bat should have been called, otherwise defaults are used.
rem PATH must contain Qt5's binary location
rem

rem Couldn't get the following line to work if INCLUDE is not defined; https://stackoverflow.com/questions/39359457/sub-string-expansion-with-empty-string-causes-error-in-if-clause
rem if x%INCLUDE:Microsoft Visual Studio=% == x%INCLUDE% (

set MSVC_CALLED=false
echo %INCLUDE% | findstr /i /c:"Microsoft Visual Studio" >nul 2>&1 && set MSVC_CALLED=true
echo MSVC_CALLED=%MSVC_CALLED%
if not "%MSVC_CALLED%" == "true" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
    set ZLIB_ROOT=C:\local\zlib_13
    set BOOST_ROOT=C:\local\boost_1_83_0
)

rem ==================== begin ChatGPT generated ====================
rem Figure out where Qt5 is installed

setlocal enabledelayedexpansion

REM Create a temporary copy of PATH
set tempPath=%PATH%
rem set QT_ROOT_TMP=""

:loop
REM Extract the portion of PATH up to the first semicolon
for /f "tokens=1* delims=;" %%a in ("!tempPath!") do (
    set currentPath=%%a
    set tempPath=%%b
)

REM Check if the current path contains the desired substring
echo !currentPath! | findstr /C:"\Qt\5" >nul
if !errorlevel! == 0 (
    rem echo Entry that matches YourStringHere: !currentPath!
	set QT_ROOT_TMP=!currentPath!\..
)

REM Check if there's more to process in the temporary PATH
if not "!tempPath!"=="" goto loop

rem echo found dir: %QT_ROOT_TMP%

endlocal & set QT_ROOT=%QT_ROOT_TMP%

rem ==================== end ChatGPT generated ====================



echo QT_ROOT: %QT_ROOT%
rem if "%PATH:\Qt\5=%" == "%PATH%"
if "%QT_ROOT%" == "" echo The build process must start from a "Qt 5 command prompt", which should generate a "\Qt\5" entry in PATH, which wasn't found & exit /B
echo VCToolsVersion: %VCToolsVersion%
if not defined VCToolsVersion echo You must initialize the MSVC environment, by running something like 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64' & exit /B
echo ZLIB_ROOT: %ZLIB_ROOT%
if not defined ZLIB_ROOT echo ZLIB_ROOT must be defined & exit /B
echo BOOST_ROOT: %BOOST_ROOT%
if not defined BOOST_ROOT echo BOOST_ROOT must be defined & exit /B

rem exit /B


mkdir build 2> nul

rem Generate the project files in the output directory.
pushd build

@REM mkdir Release 2> nul

rem -A x64
cmake -G "Visual Studio 17 2022" ../
if errorlevel 1 popd & exit /B
cmake --build . --config Release -j %NUMBER_OF_PROCESSORS%
if errorlevel 1 popd & exit /B

echo on

popd

echo.
echo If all went OK, a working executable should be in build\Release\dist
echo.

exit /B
