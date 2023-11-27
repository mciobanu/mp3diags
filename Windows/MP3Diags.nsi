; Based on several NSIS examples, including http://nsis.sourceforge.net/Run_an_application_shortcut_after_an_install
!include "MUI2.nsh"

; Some defines
!define PRODUCT_NAME "MP3 Diags"
!define PRODUCT_DIR "MP3Diags"
!define PRODUCT_UNINSTALL "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_DIR}"

; The name of the installer
Name "${PRODUCT_NAME}"

; The file to write
OutFile "MP3DiagsSetup.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\${PRODUCT_DIR}"
;InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MP3Diags" "UninstallString"
InstallDirRegKey HKLM "${PRODUCT_UNINSTALL}" "UninstallString"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

SetCompressor /SOLID lzma

 Var StartMenuFolder

!define MUI_ABORTWARNING

;--------------------------------
; Pages

;Page license
;Page directory
;Page components
;Page instfiles

;--------------------------------
; Display License
;LicenseData "gplv2.txt"


  !insertmacro MUI_PAGE_LICENSE "gplv2.txt"
;  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY

  ;Start Menu Folder Page Configuration
;  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
;  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Modern UI Test"
;  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES

    !define MUI_FINISHPAGE_NOAUTOCLOSE
    !define MUI_FINISHPAGE_RUN
    !define MUI_FINISHPAGE_RUN_CHECKED
    !define MUI_FINISHPAGE_RUN_TEXT "Run ${PRODUCT_NAME}"
    !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"
  !insertmacro MUI_PAGE_FINISH



  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES






!insertmacro MUI_LANGUAGE "English"

Section "Main Application" !Required ;No components page, name is not important
  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Put file there
  File MP3DiagsWindows.exe
  File favicon.ico

  File *.qm
  File *.dll
  File *.txt

  SetOutPath $INSTDIR\iconengines
  File iconengines\qsvgicon.dll

  SetOutPath $INSTDIR\imageformats
  File imageformats\qgif.dll
  File imageformats\qicns.dll
  File imageformats\qico.dll
  File imageformats\qjpeg.dll
  File imageformats\qsvg.dll
  File imageformats\qtga.dll
  File imageformats\qtiff.dll
  File imageformats\qwbmp.dll
  File imageformats\qwebp.dll

  SetOutPath $INSTDIR\platforms
  File platforms\qwindows.dll

  SetOutPath $INSTDIR\styles
  File styles\qwindowsvistastyle.dll

  SetOutPath $INSTDIR\translations
  File translations\qt_cs.qm
  File translations\qt_de.qm
  File translations\qt_en.qm
  File translations\qt_fr.qm


  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\MP3 Diags.lnk" "$INSTDIR\MP3DiagsWindows.exe" \
      "" "$INSTDIR\favicon.ico" 0 SW_SHOWNORMAL
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END

  FileOpen $4 "$INSTDIR\MP3DiagsCLI.cmd" w
  FileWrite $4 "@echo off$\r$\n"
  FileWrite $4 "$\"$INSTDIR\MP3DiagsWindows.exe$\" %* > %TEMP%\Mp3DiagsOut.txt$\r$\n"
  FileWrite $4 "type %TEMP%\Mp3DiagsOut.txt$\r$\n"
  FileWrite $4 "del %TEMP%\Mp3DiagsOut.txt$\r$\n"
  FileClose $4

  ; Tell the compiler to write an uninstaller and to look for a "Uninstall" section
  WriteUninstaller $INSTDIR\Uninstall.exe

  WriteRegStr HKLM "${PRODUCT_UNINSTALL}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${PRODUCT_UNINSTALL}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
  ;WriteRegStr HKLM "${PRODUCT_UNINSTALL}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"

SectionEnd ; end the section


; The uninstall section
Section "un.Uninstall"

  Delete $INSTDIR\*.exe
  Delete $INSTDIR\*.txt
  Delete $INSTDIR\*.dll
  Delete $INSTDIR\*.qm
  Delete $INSTDIR\*.cmd
  Delete $INSTDIR\*.ico

  Delete $INSTDIR\iconengines\*.dll
  Delete $INSTDIR\imageformats\*.dll
  Delete $INSTDIR\platforms\*.dll
  Delete $INSTDIR\styles\*.dll
  Delete $INSTDIR\translations\*.qm

  ;Delete $INSTDIR\Uninstall.exe
  ;Delete $INSTDIR\MP3DiagsWindows.exe
  ;Delete $INSTDIR\favicon.ico

  ;Delete $INSTDIR\boost.txt
  ;Delete $INSTDIR\libboost_serialization-*.dll
  ;Delete $INSTDIR\libboost_program_options-*.dll
  ;; boost_serialization-*.dll might be there from an older version
  ;Delete $INSTDIR\boost_serialization-*.dll
  ;Delete $INSTDIR\changelog.txt
  ;Delete $INSTDIR\gplv2.txt
  ;Delete $INSTDIR\gplv3.txt
  ;Delete $INSTDIR\lgpl-2.1.txt
  ;Delete $INSTDIR\lgplv3.txt
  ;Delete $INSTDIR\libgcc_s_dw2-1.dll
  ;;Delete $INSTDIR\mingwm10.dll
  ;Delete $INSTDIR\Qt*.dll
  ;Delete $INSTDIR\zlib.txt
  ;Delete $INSTDIR\zlib1.dll
  ;Delete $INSTDIR\*.qm
  ;Delete $INSTDIR\MP3DiagsCLI.cmd

  ;Delete $INSTDIR\iconengines\qsvgicon4.dll
  ;Delete $INSTDIR\imageformats\qsvg4.dll
  ;Delete $INSTDIR\imageformats\qjpeg4.dll
  ;Delete $INSTDIR\imageformats\qgif4.dll

  RMDir $INSTDIR\iconengines
  RMDir $INSTDIR\imageformats
  RMDir $INSTDIR\platforms
  RMDir $INSTDIR\styles
  RMDir $INSTDIR\translations
  RMDir $INSTDIR

  DeleteRegKey HKEY_CLASSES_ROOT "Directory\shell\mp3diags_temp_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Drive\shell\mp3diags_temp_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Directory\shell\mp3diags_visible_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Drive\shell\mp3diags_visible_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Directory\shell\mp3diags_hidden_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Drive\shell\mp3diags_hidden_dir"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

  Delete "$SMPROGRAMS\$StartMenuFolder\MP3 Diags.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"

  DeleteRegKey HKLM "${PRODUCT_UNINSTALL}"

SectionEnd

Function LaunchLink
;  ExecShell "" "$SMPROGRAMS\$StartMenuFolder\MP3 Diags.lnk"
  ExecShell "" "$INSTDIR\MP3DiagsWindows.exe"
FunctionEnd
