; Based on several NSIS examples, including http://nsis.sourceforge.net/Run_an_application_shortcut_after_an_install
; Newer documentation: https://nsis.sourceforge.io/Docs/Modern%20UI/Readme.html
!include "MUI2.nsh"

; Some defines
!define PRODUCT_NAME "MP3 Diags Unstable" ; ttt0 isolate "Unstable" in dedicated variable
!define PRODUCT_DIR "MP3Diags-unstable"
!define PRODUCT_UNINSTALL "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_DIR}"

; The name of the installer
Name "${PRODUCT_NAME}"

; The file to write
OutFile "MP3DiagsSetup-unstable.exe"

; The default installation directory
InstallDir "$PROGRAMFILES64\${PRODUCT_DIR}"
;InstallDirRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MP3Diags" "UninstallString"
InstallDirRegKey HKLM "${PRODUCT_UNINSTALL}" "UninstallString"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

SetCompressor /SOLID lzma

!define sourceDir "out\build\x64-release"

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

;Section  ; !!! no Section - causes an error
  !insertmacro MUI_PAGE_LICENSE "${sourceDir}\gplv2.txt"
;  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY

  ;Start Menu Folder Page Configuration
;  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
;  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Modern UI Test"
;  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES

    ; indented instructions define parameters for MUI_PAGE_FINISH
    !define MUI_FINISHPAGE_NOAUTOCLOSE
    ;!define MUI_FINISHPAGE_RUN
    ;!define MUI_FINISHPAGE_RUN_CHECKED
    ;!define MUI_FINISHPAGE_RUN_TEXT "Run ${PRODUCT_NAME}"
    ;!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"
    !define MUI_FINISHPAGE_TEXT "You can start MP3 Diags from the start menu or the desktop shortcut (if created)."
    !define MUI_FINISHPAGE_SHOWREADME ""
    #!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
    !define MUI_FINISHPAGE_SHOWREADME_TEXT "Create Desktop Shortcut"
    !define MUI_FINISHPAGE_SHOWREADME_FUNCTION finishPageAction
  !insertmacro MUI_PAGE_FINISH


  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
;SectionEnd



!insertmacro MUI_LANGUAGE "English"

; https://stackoverflow.com/questions/62092185/how-to-install-the-visual-c-redist-using-nsis
Section "Visual Studio Runtime"
  SetOutPath "$INSTDIR"
  File ${sourceDir}\vc_redist.x64.exe
  ;ExecWait "$INSTDIR\vc_redist.x64.exe"
  ExecWait '"$INSTDIR\vc_redist.x64.exe" /quiet' ; !!! The reason for "quiet" is that the dialog shown is confusing. The assumption is that it doesn't downgrade existing installs; ttt1 Make sure this is true
  Delete "$INSTDIR\vc_redist.x64.exe"
SectionEnd


Section "Main Application" !Required ;No components page, name is not important
  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ;DetailPrint "NSIS=${NSIS_VERSION}"
  ;DetailPrint "INSTDIR=$INSTDIR"

  ; Put file there
  File ${sourceDir}\MP3DiagsWindows-unstable.exe
  File ${sourceDir}\favicon.ico

  File ${sourceDir}\*.qm
  File ${sourceDir}\*.dll
  File /x CMake* /x VS* ${sourceDir}\*.txt

  SetOutPath $INSTDIR\iconengines
  File ${sourceDir}\iconengines\*.dll

  SetOutPath $INSTDIR\imageformats
  File ${sourceDir}\imageformats\*.dll

  SetOutPath $INSTDIR\platforms
  File ${sourceDir}\platforms\*.dll

  SetOutPath $INSTDIR\styles
  File ${sourceDir}\styles\*.dll


  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    CreateDirectory "$SMPROGRAMS\$StartMenuFolder" ; !!! there's a checkbox to not create shortcuts, and it works fine
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\MP3 Diags Unstable.lnk" "$INSTDIR\MP3DiagsWindows-unstable.exe" \
      "" "$INSTDIR\favicon.ico" 0 SW_SHOWNORMAL
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe" ; ttt1 seems to no longer work; probably remove
  !insertmacro MUI_STARTMENU_WRITE_END

  ; Create custom version for MP3DiagsCLI-unstable.cmd, to include the exe path ; ttt0 generate in MakeArchives.sh
  FileOpen $4 "$INSTDIR\MP3DiagsCLI-unstable.cmd" w
  FileWrite $4 "@echo off$\r$\n"
  FileWrite $4 "$\"$INSTDIR\MP3DiagsWindows-unstable.exe$\" %* > %TEMP%\Mp3DiagsOut.txt$\r$\n"
  FileWrite $4 "type %TEMP%\Mp3DiagsOut.txt$\r$\n"
  FileWrite $4 "del %TEMP%\Mp3DiagsOut.txt$\r$\n"
  FileClose $4

  ; Tell the compiler to write an uninstaller and to look for a "Uninstall" section
  WriteUninstaller $INSTDIR\Uninstall.exe

  ; Computer\HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\MP3Diags-unstable (different for 32 vs. 64 bit)
  WriteRegStr HKLM "${PRODUCT_UNINSTALL}" "DisplayName" "${PRODUCT_NAME}"
  WriteRegStr HKLM "${PRODUCT_UNINSTALL}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
  ;WriteRegStr HKLM "${PRODUCT_UNINSTALL}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
  ;WriteRegStr ${PURK} "${PUK}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${PRODUCT_UNINSTALL}" "Publisher" "Ciobi"
  WriteRegStr HKLM "${PRODUCT_UNINSTALL}" "DisplayIcon" "$\"$INSTDIR\favicon.ico$\""

SectionEnd


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

  Delete "$desktop\MP3 Diags Unstable.lnk"


  RMDir $INSTDIR\iconengines
  RMDir $INSTDIR\imageformats
  RMDir $INSTDIR\platforms
  RMDir $INSTDIR\styles
  RMDir $INSTDIR

  DeleteRegKey HKEY_CLASSES_ROOT "Directory\shell\mp3diags_temp_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Drive\shell\mp3diags_temp_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Directory\shell\mp3diags_visible_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Drive\shell\mp3diags_visible_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Directory\shell\mp3diags_hidden_dir"
  DeleteRegKey HKEY_CLASSES_ROOT "Drive\shell\mp3diags_hidden_dir"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder

  Delete "$SMPROGRAMS\$StartMenuFolder\MP3 Diags Unstable.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"

  DeleteRegKey HKLM "${PRODUCT_UNINSTALL}"

SectionEnd

; ttt0 See about putting this back. The problem is that the program only sees the C: drive when starting from the setup. Something that may be related is that
; when going to Desktop in FileExplorer, only MP3Digs' icon is there, while the others are not, probably because the others are installed for all users.
;Function LaunchLink
;;  ExecShell "" "$SMPROGRAMS\$StartMenuFolder\MP3 Diags.lnk"
;  ExecShell "" "$INSTDIR\MP3DiagsWindows-unstable.exe"
;FunctionEnd

; https://stackoverflow.com/questions/1517471/how-to-add-a-desktop-shortcut-option-on-finish-page-in-nsis-installer
Function finishPageAction
  CreateShortcut "$desktop\MP3 Diags Unstable.lnk" "$INSTDIR\MP3DiagsWindows-unstable.exe" "" "$INSTDIR\favicon.ico" 0 SW_SHOWNORMAL ; ttt0 use a higher-res icon
FunctionEnd
