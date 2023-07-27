#define MyAppName "WinMute"
#define MyAppVersion "2.3.1.0"
#define MyAppPublisher "LX-Systems"
#define MyAppURL "https://www.lx-s.de/winmute"
#define MyAppExeName "WinMute.exe"
#define MyAppMutex "LxSystemsWinMute"

[Setup]
AppId={{D2E8F9EF-11E7-418B-B9B7-A35A69A30490}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppPublisher}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\bin\license.txt
InfoBeforeFile=info-before.txt
AppMutex={#MyAppMutex}
SetupMutex={#MyAppMutex}Setup
PrivilegesRequiredOverridesAllowed=dialog
PrivilegesRequired=lowest
OutputDir=..\bin\
OutputBaseFilename=WinMuteSetup
SetupIconFile=..\..\WinMute\icons\app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}.exe
ArchitecturesInstallIn64BitMode=x64
Compression=lzma
SolidCompression=yes
WizardStyle=modern
ShowLanguageDialog=auto

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart" ; Description: "Start WinMute when you log on" ; GroupDescription: "Autostart"; Flags: unchecked

[Files]
Source: "..\bin\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\changelog.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\license.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\liesmich.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\readme.html"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\lx-systems"; Flags: uninsdeletekeyifempty
Root: HKCU; Subkey: "Software\lx-systems\WinMute"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\lx-systems\WinMute"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"
Root: HKCU; Subkey: "Software\lx-systems\WinMute\BluetoothDevices"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\lx-systems\WinMute\WifiNetworks"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\lx-systems\WinMute\ManagedAudioEndpoints"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "LX-Systems WinMute"; ValueData: "{app}\{#MyAppExeName}"; Tasks: autostart

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
