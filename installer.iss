[Setup]
AppName=Winlator Desktop & Taskbar Pro
AppVersion=1.0.0
AppPublisher=Winlator Community
DefaultDirName={autopf}\WinlatorShell
DefaultGroupName=Winlator Desktop & Taskbar Pro
OutputDir=Output
OutputBaseFilename=WinlatorShell_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
WizardSizePercent=120
DisableWelcomePage=no
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "autorun"; Description: "Start automatically when Winlator container starts"; GroupDescription: "Winlator Startup Options:"

[Files]
Source: "build\WinlatorShell.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Winlator Desktop Shell"; Filename: "{app}\WinlatorShell.exe"
Name: "{autodesktop}\Winlator Desktop Shell"; Filename: "{app}\WinlatorShell.exe"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "WinlatorShellPro"; ValueData: """{app}\WinlatorShell.exe"""; Flags: uninsdeletevalue; Tasks: autorun

[Run]
Filename: "{app}\WinlatorShell.exe"; Description: "{cm:LaunchProgram,Winlator Desktop & Taskbar Pro}"; Flags: nowait postinstall skipifsilent