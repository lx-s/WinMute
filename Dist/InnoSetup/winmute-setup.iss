
#include               ".\winmute-setup-languages.iss"

#define MyAppName      "WinMute"
#define MyAppExeName   "..\bin\" + MyAppName + ".exe"
#define MyAppVersion   GetVersionNumbersString(MyAppExeName)
#define MyAppPublisher "LX-Systems"
#define MyAppURL       "https://www.lx-s.de/winmute"
#define MyAppExeName   "WinMute.exe"
#define MyAppMutex     "LxSystemsWinMute"
#define CurrentYear    GetDateTimeString('yyyy','','')

[Setup]
AppId={{D2E8F9EF-11E7-418B-B9B7-A35A69A30490}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}

AppCopyright=(c) {#CurrentYear} {#MyAppPublisher}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

VersionInfoDescription={#MyAppName} installer
VersionInfoProductName={#MyAppName}
VersionInfoVersion={#MyAppVersion}

UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}.exe

WizardStyle=modern
ShowLanguageDialog=yes
UsePreviousLanguage=no
LanguageDetectionMethod=uilanguage

DefaultDirName={autopf}\{#MyAppPublisher}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\bin\license.rtf
InfoBeforeFile=info-before.txt
AppMutex={#MyAppMutex}
SetupMutex={#MyAppMutex}Setup
;PrivilegesRequiredOverridesAllowed=dialog
PrivilegesRequired=lowest
OutputDir=..\bin\
OutputBaseFilename=WinMuteSetup

SetupIconFile=..\..\WinMute\icons\app.ico
ArchitecturesInstallIn64BitMode=x64
Compression=lzma
SolidCompression=yes

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart" ; Description: "{cm:StartpAppLogon}" ; GroupDescription: "Autostart"; Flags: unchecked

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

