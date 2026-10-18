; Audiosphere Windows installer (Inno Setup 6)
;
; Build with:
;   ISCC.exe /DAppVersion=0.1.0 /DBuildDir=<...>\build\Audiosphere_artefacts\Release /DRepoDir=<repo> Audiosphere.iss

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build\Audiosphere_artefacts\Release"
#endif
#ifndef RepoDir
  #define RepoDir "..\.."
#endif

#define AppName "Audiosphere"
#define AppPublisher "Audiosphere"
#define AppURL "https://github.com/Bobanes1/Audiosphere_Plugin"

[Setup]
AppId={{6F1D3C52-8B7E-4C1A-9E43-0A55D2B8F7A1}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}/issues
AppUpdatesURL={#AppURL}/releases
VersionInfoVersion={#AppVersion}
VersionInfoDescription={#AppName} Installer
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
DisableDirPage=auto
LicenseFile={#RepoDir}\LICENSE
OutputBaseFilename=Audiosphere-{#AppVersion}-Windows-Setup
SetupIconFile=audiosphere.ico
UninstallDisplayIcon={app}\audiosphere.ico
UninstallDisplayName={#AppName} {#AppVersion}
WizardStyle=modern
WizardImageFile=wizard-large.bmp
WizardSmallImageFile=wizard-small.bmp
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
MinVersion=10.0

[Types]
Name: "full"; Description: "Full installation (recommended)"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in (Ableton Live, FL Studio, Bitwig, Reaper, Cubase...)"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "Standalone application"; Types: full

[Files]
; The VST3 bundle goes to the system-wide VST3 folder every host scans.
Source: "{#BuildDir}\VST3\Audiosphere.vst3\*"; DestDir: "{commoncf64}\VST3\Audiosphere.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#BuildDir}\Standalone\Audiosphere.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone
Source: "{#RepoDir}\README.md"; DestDir: "{app}"; DestName: "README.txt"; Flags: ignoreversion
Source: "{#RepoDir}\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "audiosphere.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Audiosphere"; Filename: "{app}\Audiosphere.exe"; Components: standalone
Name: "{group}\Read Me"; Filename: "{app}\README.txt"
Name: "{group}\Uninstall Audiosphere"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Audiosphere"; Filename: "{app}\Audiosphere.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for the standalone app"; Components: standalone; Flags: unchecked

[Run]
Filename: "{app}\Audiosphere.exe"; Description: "Launch the Audiosphere standalone app"; Flags: nowait postinstall skipifsilent unchecked; Components: standalone

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Audiosphere.vst3"

[Messages]
FinishedLabel=Audiosphere has been installed.%n%nThe VST3 plug-in is in C:\Program Files\Common Files\VST3.%n%nIn Ableton Live 12: Options > Preferences > Plug-Ins, switch on "Use VST3 Plug-In System Folders" and click Rescan. Audiosphere then appears under Plug-Ins > VST3.%n%nYour own presets are saved to Documents\Audiosphere\Presets.
