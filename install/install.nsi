; install.msi
;
; Installer for Meridian 59 client
;--------------------------------
; NSIS plugins required:
; http://nsis.sourceforge.net/UAC_plug-in
; http://nsis.sourceforge.net/FontName_plug-in
;
; Change instances of Meridian 105 and Server 105 to your server name/number
; to avoid conflicts. Use $PROGRAMFILES\Open Meridian\Meridian servernum as
; install directory for compatibility with the Open Meridian Patcher.

!include FontReg.nsh
!include FontName.nsh
!include WinMessages.nsh
!include UAC.nsh

!define SOURCEDIR "..\run\localclient"

; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\English.nlf"
LoadLanguageFile "${NSISDIR}\Contrib\Language files\German.nlf"

; The name of the installer
Name "Meridian 59 Server 105"

; The file to write
OutFile "meridian59_server105_install.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\Open Meridian\Meridian 105"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Meridian 105" "Install_Dir"

; Compress to the max
SetCompressor /SOLID LZMA

; We run an outer script with user permission, and an inner one with admin.  See
; http://nsis.sourceforge.net/UAC_plug-in
RequestExecutionLevel user

Function .OnInit
 
UAC_Elevate:
    !insertmacro UAC_RunElevated
    StrCmp 1223 $0 UAC_ElevationAborted ; UAC dialog aborted by user?
    StrCmp 0 $0 0 UAC_Err ; Error?
    StrCmp 1 $1 0 UAC_Success ;Are we the real deal or just the wrapper?
    Quit
 
UAC_Err:
    MessageBox mb_iconstop "Unable to elevate, error $0"
    Abort
 
UAC_ElevationAborted:
    # elevation was aborted, run as normal?
    MessageBox mb_iconstop "This installer requires admin access, aborting!"
    Abort
 
UAC_Success:
    StrCmp 1 $3 +4 ;Admin?
    StrCmp 3 $1 0 UAC_ElevationAborted ;Try again?
    MessageBox mb_iconstop "This installer requires admin access, try again"
    goto UAC_Elevate 
 
FunctionEnd


Function .OnInstFailed
FunctionEnd

; Launch Meridian on installer close.
Function .OnInstSuccess
!insertmacro UAC_AsUser_ExecShell "" "meridian.exe" "" "$INSTDIR\" ""
FunctionEnd
;--------------------------------

; Pages

; Leave license out, not sure what we should be putting here.
; Page license 
Page components
; We should be installing to a known default directory.
; Keeps compatibility with the Open Meridian Patcher.
; Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Meridian 59 (required)"

  SectionIn RO

  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Put executable files there
  File "${SOURCEDIR}\club.exe"
  File "${SOURCEDIR}\heidelb1.ttf"
  File "${SOURCEDIR}\license.rtf"
  File "${SOURCEDIR}\m59bind.exe"
  File "${SOURCEDIR}\meridian.exe"
  File "${SOURCEDIR}\irrKlang.dll"
  File "${SOURCEDIR}\ikpMP3.dll"

  ; Install font
  StrCpy $FONT_DIR $FONTS
  !insertmacro InstallTTFFont "${SOURCEDIR}\heidelb1.ttf"
  SendMessage ${HWND_BROADCAST} ${WM_FONTCHANGE} 0 0 /TIMEOUT=5000

  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\Meridian 105" "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Meridian 59 Server 105" "DisplayName" "Meridian 59 Server 105"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Meridian 59 Server 105" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Meridian 59 Server 105" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Meridian 59 Server 105" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  ; Copy resources
  SetOutPath "$INSTDIR\resource"
  ;File ${SOURCEDIR}\resource\*.*
  File "${SOURCEDIR}\resource\rsc0000.rsb"
  File "${SOURCEDIR}\resource\splash.bgf"
  File "${SOURCEDIR}\resource\Main.ogg"
  File "${SOURCEDIR}\resource\Login.ogg"
  File "${SOURCEDIR}\resource\intro.dll"
  File "${SOURCEDIR}\resource\char.dll"
  File "${SOURCEDIR}\resource\merintr.dll"
  File "${SOURCEDIR}\resource\stats.dll"
  File "${SOURCEDIR}\resource\dm.dll"
  File "${SOURCEDIR}\resource\admin.dll"
  File "${SOURCEDIR}\resource\mailnews.dll"
  File "${SOURCEDIR}\resource\chess.dll"
  SetOutPath "$INSTDIR\mail"
  File "${SOURCEDIR}\mail\game.map"
  SetOutPath "$INSTDIR\de"
  File "${SOURCEDIR}\de\*.*"

  ; Create extra directories now; creating them on demand seems to fail
  ; under some UAC conditions.
  CreateDirectory "$INSTDIR\download"
  CreateDirectory "$INSTDIR\ads"
  CreateDirectory "$INSTDIR\help"
SectionEnd

; Optional section (can be disabled by the user)
Section "Desktop Shortcut"
  ; Set to run in data directory
  SetOutPath $INSTDIR
  CreateShortCut "$DESKTOP\Meridian 59 Server 105.lnk" "$INSTDIR\meridian.exe" "" "$INSTDIR\meridian.exe" 0
SectionEnd

; Optional section
Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\OpenMeridian"
  SetOutPath $INSTDIR
  CreateShortCut "$SMPROGRAMS\OpenMeridian\Meridian 59 Server 105.lnk" "$INSTDIR\meridian.exe" "" "$INSTDIR\meridian.exe" 0
SectionEnd

;--------------------------------

; Uninstaller

Function un.OnInit
UAC_Elevate:
    !insertmacro UAC_RunElevated
    StrCmp 1223 $0 UAC_ElevationAborted ; UAC dialog aborted by user?
    StrCmp 0 $0 0 UAC_Err ; Error?
    StrCmp 1 $1 0 UAC_Success ;Are we the real deal or just the wrapper?
    Quit
 
UAC_Err:
    MessageBox mb_iconstop "Unable to elevate, error $0"
    Abort
 
UAC_ElevationAborted:
    # elevation was aborted, run as normal?
    MessageBox mb_iconstop "This installer requires admin access, aborting!"
    Abort
 
UAC_Success:
    StrCmp 1 $3 +4 ;Admin?
    StrCmp 3 $1 0 UAC_ElevationAborted ;Try again?
    MessageBox mb_iconstop "This installer requires admin access, try again"
    goto UAC_Elevate 
 
FunctionEnd

Function un.OnUnInstFailed
FunctionEnd
 
Function un.OnUnInstSuccess
FunctionEnd

Section "Uninstall"
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Meridian 59 Server 105"
  DeleteRegKey HKLM "SOFTWARE\Meridian 105"

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\OpenMeridian\Meridian 59 Server 105.lnk"
  Delete "$DESKTOP\Meridian 59 Server 105.lnk"

  ; Remove directories used
  RMDir /r "$INSTDIR"
SectionEnd
