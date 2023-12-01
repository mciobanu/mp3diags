@echo off

rem Helper for CMake, making sure everything is properly setup.
rem
rem Creates some dirs, calls cmake, copies some files, so in the end the executable in build\Release\dist
rem should have all its dependencies and be able to run, while the NSIS installer has the files it needs.
rem Not strictly needed. You can run CMake by its own, as long as the environment variables are set.
rem
rem Requirements:
rem Visual Studio, Qt5, Boost, and ZLib need to be installed.
rem Must be started from a Qt "command prompt". PATH must contain Qt5's binary location.
rem
rem In simple settings, Boost_ROOT and ZLIB_ROOT environment variables can be determined automatically if they are
rem not defined, and Visual Studio's vcvarsall.bat gets called. For Visual Studio other than 2022 Community, it must
rem be called before running this. Also, ZLIB_ROOT should be set if ZLib is not installed in C:\local or there
rem are multiple versions, and the same for BOOST_ROOT, but if it gets to this there's little point in running
rem this script and CMake can be used directly.


rem ttt1 Visual Studio version and architecture are hardcoded in some places


rem ----------------- MSVC -----------------

rem Couldn't get the following line to work if INCLUDE is not defined; https://stackoverflow.com/questions/39359457/sub-string-expansion-with-empty-string-causes-error-in-if-clause
rem if x%INCLUDE:Microsoft Visual Studio=% == x%INCLUDE% (
rem set MSVC_CALLED=false
rem echo %INCLUDE% | findstr /i /c:"Microsoft Visual Studio" >nul 2>&1 && set MSVC_CALLED=true
rem echo MSVC_CALLED=%MSVC_CALLED%
rem if not "%MSVC_CALLED%" == "true" (
rem     call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
rem )


for /f "tokens=*" %%i in ('reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\devenv.exe"') do set MP3DIAGS_DEV_ENV=%%i
rem echo MP3DIAGS_DEV_ENV: %MP3DIAGS_DEV_ENV%
rem https://ss64.com/nt/syntax-substring.html
set MP3DIAGS_VC_VARS=%MP3DIAGS_DEV_ENV:~24,-23%VC\Auxiliary\Build\vcvarsall.bat
rem echo MP3DIAGS_VC_VARS: %MP3DIAGS_VC_VARS%
rem ttt1 See about setlocal enabledelayedexpansion & endlocal. They don't seem to work well inside "if". Also, MSVC needs variables set

if not defined VCToolsVersion (
    rem call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64
    call "%MP3DIAGS_VC_VARS%" amd64
    if errorlevel 1 exit /B
)

echo VCToolsVersion: %VCToolsVersion%
if not defined VCToolsVersion echo You must initialize the MSVC environment, by running something like 'call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64' & exit /B



rem ----------------- ZLib -----------------
if not defined ZLIB_ROOT (
    rem set ZLIB_ROOT=C:\local\zlib_13
    for /f %%i in ('dir /B /A:D "C:\local\zlib*"') do set ZLIB_ROOT=C:\local\%%i
    rem ttt1 Add support for multiple versions
)
echo ZLIB_ROOT: %ZLIB_ROOT%
if not defined ZLIB_ROOT echo ZLIB_ROOT must be defined & exit /B



rem ----------------- Boost -----------------
if not defined BOOST_ROOT (
    rem set BOOST_ROOT=C:\local\boost_1_83_0
    for /f %%i in ('dir /B /A:D "C:\local\boost*"') do set BOOST_ROOT=C:\local\%%i
)
echo BOOST_ROOT: %BOOST_ROOT%
if not defined BOOST_ROOT echo BOOST_ROOT must be defined & exit /B



rem ----------------- Qt -----------------

rem ----- begin ChatGPT generated -----
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

rem ----- end ChatGPT generated -----



echo QT_ROOT: %QT_ROOT%
rem if "%PATH:\Qt\5=%" == "%PATH%"
if "%QT_ROOT%" == "" echo The build process must start from a "Qt 5 command prompt", which should generate a "\Qt\5" entry in PATH, which wasn't found & exit /B

rem exit /B


rem ----------------- Run the build process -----------------


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
