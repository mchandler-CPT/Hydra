[Setup]
; Generated a brand-new unique GUID specifically for Hydra to prevent registry clashes with Rumble
AppId={{C9B3D1A5-3F7A-4E9B-9C0D-8B1A2F3E4D5C}
AppName=The Hydra
AppVersion=1.0.0-beta
AppPublisher=Mark Chandler / Ant
PrivilegesRequired=admin
; 64-bit hosts must install VST3 under native Common Files (not WOW64 Program Files (x86)).
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64
OutputDir=Output
OutputBaseFilename=Hydra_Setup
DefaultDirName={autopf}\The Hydra
DefaultGroupName=The Hydra
DisableDirPage=yes
DisableProgramGroupPage=yes
Compression=zip
SolidCompression=no
VersionInfoCompany=Mark Chandler / Ant
VersionInfoDescription=The Hydra Synthesizer Installer
VersionInfoVersion=1.0.0.0
WizardStyle=modern

; Optional: uncomment and point to your icon if available.
; SetupIconFile=assets\hydra.ico

[Tasks]
Name: "desktopmanual"; Description: "Create a Desktop Shortcut for the README manual"; GroupDescription: "Additional Icons:"

[Files]
; VST3 bundle (recursive copy from Hydra-specific staging area)
Source: "installer_dist\Hydra.vst3\*"; DestDir: "{commoncf}\VST3\Hydra.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

; Presets
Source: "installer_dist\Presets\*"; DestDir: "{userappdata}\The Hydra\Presets"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; Optional PDF manual
Source: "installer_dist\README.pdf"; DestDir: "{userappdata}\The Hydra"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
; Desktop shortcut to the Hydra manual PDF
Name: "{autodesktop}\The Hydra README"; Filename: "{userappdata}\The Hydra\README.pdf"; Tasks: desktopmanual