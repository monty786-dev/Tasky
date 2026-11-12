; installer.nsi - NSIS installer for WLC Taskbar
; Build: makensis installer.nsi
Unicode true
Name "WLC Taskbar"
OutFile "WLC-Taskbar-Setup.exe"
InstallDir "$PROGRAMFILES64\WLC Taskbar"
RequestExecutionLevel admin

!define APPVER "1.0.0"

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "Win32 Bridge (Wine)" SecBridge
  SectionIn RO
  SetOutPath "$INSTDIR"
  File "build\Win32Bridge.exe"
  File "build\WindowLookChanger.exe"
  CreateDirectory "$INSTDIR\themes"
  WriteRegStr HKCU "Software\WLC Taskbar" "BridgeExe" "$INSTDIR\Win32Bridge.exe"

  ; add to Wine's startup via Run key in the current user hive
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "WLCTaskbar" "$INSTDIR\Win32Bridge.exe"
SectionEnd

Section "Hide Wine explorer taskbar" SecReg
  WriteRegDWORD HKCU "Software\Wine\Explorer" "ShowSystray" 0
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Policies\Explorer" "NoTrayItemsDisplay" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Policies\Explorer" "NoToolbarsOnTaskbar" 1
SectionEnd

Section "Box64 performance profile" SecBox64
  ; written as plain text - the Android side copies it into the container
  SetOutPath "$INSTDIR"
  File "build\wine_tune.sh"
  File "build\box64_env.txt"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\Win32Bridge.exe"
  Delete "$INSTDIR\WindowLookChanger.exe"
  RMDir /r "$INSTDIR\themes"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "WLCTaskbar"
  DeleteRegKey HKCU "Software\WLC Taskbar"
  RMDir "$INSTDIR"
SectionEnd
