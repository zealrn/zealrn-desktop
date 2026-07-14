; Copyright (C) ZealRN contributors
; SPDX-License-Identifier: GPL-3.0-or-later

Unicode True
RequestExecutionLevel user

!include "MUI2.nsh"

!ifndef APP_SOURCE
  !error "APP_SOURCE is required"
!endif
!ifndef OUTPUT_FILE
  !error "OUTPUT_FILE is required"
!endif
!ifndef APP_ICON
  !error "APP_ICON is required"
!endif

Name "ZealRN"
OutFile "${OUTPUT_FILE}"
InstallDir "$LOCALAPPDATA\Programs\ZealRN"
InstallDirRegKey HKCU "Software\abnzrdev\ZealRN" "InstallDir"
SetCompressor /SOLID lzma

VIProductVersion "0.1.0.0"
VIAddVersionKey "ProductName" "ZealRN"
VIAddVersionKey "ProductVersion" "0.1.0"
VIAddVersionKey "FileVersion" "0.1.0"
VIAddVersionKey "CompanyName" "abnzrdev"
VIAddVersionKey "FileDescription" "ZealRN 0.1.0 Setup"
VIAddVersionKey "LegalCopyright" "GPL-3.0-or-later; upstream contributors credited in COPYING"

!define MUI_ABORTWARNING
!define MUI_ICON "${APP_ICON}"
!define MUI_UNICON "${APP_ICON}"
!define MUI_FINISHPAGE_RUN "$INSTDIR\zealrn.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run ZealRN"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${APP_SOURCE}\COPYING"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "ZealRN" MainSection
  SetShellVarContext current
  SetOutPath "$INSTDIR"
  File /r "${APP_SOURCE}\*"

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "Software\abnzrdev\ZealRN" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZealRN" "DisplayName" "ZealRN"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZealRN" "DisplayVersion" "0.1.0"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZealRN" "Publisher" "abnzrdev"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZealRN" "URLInfoAbout" "https://github.com/abnzrdev/zealrn"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZealRN" "DisplayIcon" "$INSTDIR\zealrn.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZealRN" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZealRN" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZealRN" "NoRepair" 1

  CreateShortCut "$SMPROGRAMS\ZealRN.lnk" "$INSTDIR\zealrn.exe"
SectionEnd

Section "Uninstall"
  SetShellVarContext current
  Delete "$SMPROGRAMS\ZealRN.lnk"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\ZealRN"
  DeleteRegKey HKCU "Software\abnzrdev\ZealRN"
SectionEnd
