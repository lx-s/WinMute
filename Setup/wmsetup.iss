; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "WinMute"
#define MyAppVersion "2.0.0.0"
#define MyAppPublisher "LX-Systems"
#define MyAppURL "https://www.lx-s.de/winmute"
#define MyAppExeName "WinMute.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{D2E8F9EF-11E7-418B-B9B7-A35A69A30490}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=E:\Development\Code\WinMute\Dist\bin\license.txt
InfoAfterFile=E:\Development\Code\WinMute\Dist\bin\readme.html
; Uncomment the following line to run in non administrative install mode (install for current user only.)
;PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=E:\Development\Code\WinMute\Build\Setup
OutputBaseFilename=wm-setup
SetupIconFile=E:\Development\Code\WinMute\WinMute\icons\app.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "E:\Development\Code\WinMute\Dist\bin\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Development\Code\WinMute\Dist\bin\changelog.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Development\Code\WinMute\Dist\bin\license.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Development\Code\WinMute\Dist\bin\liesmich.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Development\Code\WinMute\Dist\bin\readme.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Development\Code\WinMute\Dist\bin\ScreensaverNotify.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Development\Code\WinMute\Dist\bin\ScreensaverNotify32.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "E:\Development\Code\WinMute\Dist\bin\ScreensaverProxy32.exe"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

