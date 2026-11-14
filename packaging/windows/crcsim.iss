#ifndef AppVersion
#define AppVersion "0.0.0"
#endif
#ifndef SourceDir
#define SourceDir "..\..\stage\crcsim"
#endif
#ifndef OutputDir
#define OutputDir "..\..\dist"
#endif

[Setup]
AppId=crcsim
AppName=crcsim
AppVersion={#AppVersion}
AppVerName=crcsim {#AppVersion}
DefaultDirName={autopf}\crcsim
DefaultGroupName=crcsim
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=crcsim-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\crcsim-gui.exe
UninstallDisplayName=crcsim

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\crcsim-gui.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\crcsim.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceDir}\README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\crcsim"; Filename: "{app}\crcsim-gui.exe"
Name: "{group}\{cm:UninstallProgram,crcsim}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\crcsim"; Filename: "{app}\crcsim-gui.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\crcsim-gui.exe"; Description: "{cm:LaunchProgram,crcsim}"; Flags: nowait postinstall skipifsilent
