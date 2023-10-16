rem @echo off

rem Creates some dirs, calls cmake, copies some files, so in the end the executable in build\Release\dist
rem should have all its dependencies and be able to run.
rem
rem Requirements:
rem Visual Studio, Qt5, Boost, and ZLib need to be installed.
rem Boost_ROOT and ZLIB_ROOT environment variables need to be defined.
rem PATH must contain Qt5's binary location
rem



rem ==================== begin ChatGPT generated ====================
rem Figure out where Qt5 is installed

setlocal enabledelayedexpansion

REM Create a temporary copy of PATH
set tempPath=%PATH%
rem set QT_BIN_DIR_TMP=""

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
	set QT_BIN_DIR_TMP=!currentPath!
)

REM Check if there's more to process in the temporary PATH
rem if not "!tempPath!"=="" if "QT_BIN_DIR_TMP"=="" goto loop
if not "!tempPath!"=="" goto loop

rem echo found dir: %QT_BIN_DIR%

endlocal & set QT_BIN_DIR=%QT_BIN_DIR_TMP%

rem ==================== end ChatGPT generated ====================



echo QT_BIN_DIR: %QT_BIN_DIR%
rem if "%PATH:\Qt\5=%" == "%PATH%"
if "%QT_BIN_DIR%" == "" echo The build process must start from a "Qt 5 command prompt", which should generate a "\Qt\5" entry in PATH, which wasn't found & exit /B
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

mkdir Release 2> nul
mkdir Release\dist 2> nul
mkdir Release\dist\iconengines 2> nul
mkdir Release\dist\imageformats 2> nul
mkdir Release\dist\platforms 2> nul
mkdir Release\dist\styles 2> nul
mkdir Release\dist\translations 2> nul


rem -A x64
cmake -G "Visual Studio 17 2022" ../
if errorlevel 1 popd & exit /B
cmake --build . --config Release
if errorlevel 1 popd & exit /B

echo on

del /q Release\dist\translations\assistant*
del /q Release\dist\translations\designer*
del /q Release\dist\translations\linguist*
del /q Release\dist\translations\qtserialport*
del /q Release\dist\translations\qtwebsockets*
rem ttt1 review what else can be deleted

xcopy "%QT_BIN_DIR%\..\plugins\iconengines\qsvgicon.dll" Release\dist\iconengines /q /y
xcopy "%QT_BIN_DIR%\..\plugins\imageformats\*.dll" Release\dist\imageformats /q /y
rem remove debug dlls
del /q Release\dist\imageformats\*d.dll
xcopy "%QT_BIN_DIR%\..\plugins\platforms\qwindows.dll" Release\dist\platforms /q /y
xcopy "%QT_BIN_DIR%\..\plugins\styles\qwindowsvistastyle.dll" Release\dist\styles /q /y

xcopy "%QT_BIN_DIR%\Qt5Core.dll" Release\dist /q /y
xcopy "%QT_BIN_DIR%\Qt5Gui.dll" Release\dist /q /y
xcopy "%QT_BIN_DIR%\Qt5Svg.dll" Release\dist /q /y
xcopy "%QT_BIN_DIR%\Qt5Widgets.dll" Release\dist /q /y
xcopy "%QT_BIN_DIR%\Qt5Xml.dll" Release\dist /q /y

rem xcopy "%QT_BIN_DIR%\..\translations\qt_*.qm" Release\dist\translations /y

popd

echo.
echo If all went OK, a working executable should be in build\Release\dist
echo.

exit /B
