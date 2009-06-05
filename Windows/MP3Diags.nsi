; The name of the installer
Name "MP3 Diags"

; The file to write
OutFile "MP3DiagsSetup.exe"

; The default installation directory
InstallDir $PROGRAMFILES\MP3Diags

; Request application privileges for Windows Vista
RequestExecutionLevel admin

SetCompressor /SOLID lzma

;--------------------------------
; Pages

Page license
Page directory
Page components
Page instfiles

;--------------------------------
; Display License
LicenseData "gplv2.txt"


Section "Main Application" !Required ;No components page, name is not important
  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File Mp3DiagsWindows.exe
  File favicon.ico

  File boost.txt
  File boost_serialization-mgw34-1_39.dll
  File gplv3.txt
  File lgpl-2.1.txt
  File lgplv3.txt
  File mingwm10.dll
  File QtCore4.dll
  File QtGui4.dll
  File QtNetwork4.dll
  File QtSvg4.dll
  File QtXml4.dll
  File zlib.txt
  File zlib1.dll

  SetOutPath $INSTDIR\iconengines
  File iconengines\qsvgicon4.dll
  SetOutPath $INSTDIR\imageformats
  File imageformats\qsvg4.dll
  File imageformats\qjpeg4.dll


  CreateDirectory "$SMPROGRAMS\MP3 Diags"
  CreateShortCut "$SMPROGRAMS\MP3 Diags\MP3 Diags.lnk" "$INSTDIR\Mp3DiagsWindows.exe" \
	"" "$INSTDIR\favicon.ico" 0 SW_SHOWNORMAL
  CreateShortCut "$SMPROGRAMS\MP3 Diags\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  ; Tell the compiler to write an uninstaller and to look for a "Uninstall" section
  WriteUninstaller $INSTDIR\Uninstall.exe
  
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MP3Diags" \
                 "DisplayName" "MP3 Diags"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MP3Diags" \
                 "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
  ;WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MP3Diags" \
  ;               "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
SectionEnd ; end the section


; The uninstall section
Section "un.Uninstall"

  Delete $INSTDIR\Uninstall.exe
  Delete $INSTDIR\Mp3DiagsWindows.exe
  Delete $INSTDIR\favicon.ico

  Delete $INSTDIR\boost.txt
  Delete $INSTDIR\boost_serialization-mgw34-1_39.dll
  Delete $INSTDIR\gplv3.txt
  Delete $INSTDIR\lgpl-2.1.txt
  Delete $INSTDIR\lgplv3.txt
  Delete $INSTDIR\mingwm10.dll
  Delete $INSTDIR\QtCore4.dll
  Delete $INSTDIR\QtGui4.dll
  Delete $INSTDIR\QtNetwork4.dll
  Delete $INSTDIR\QtSvg4.dll
  Delete $INSTDIR\QtXml4.dll
  Delete $INSTDIR\zlib.txt
  Delete $INSTDIR\zlib1.dll

  Delete $INSTDIR\iconengines\qsvgicon4.dll
  Delete $INSTDIR\imageformats\qsvg4.dll
  Delete $INSTDIR\imageformats\qjpeg4.dll

  RMDir $INSTDIR\iconengines
  RMDir $INSTDIR\imageformats
  RMDir $INSTDIR

  Delete "$SMPROGRAMS\MP3 Diags\MP3 Diags.lnk"
  Delete "$SMPROGRAMS\MP3 Diags\Uninstall.lnk"
  RMDir "$SMPROGRAMS\MP3 Diags"

  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MP3Diags"

SectionEnd 
