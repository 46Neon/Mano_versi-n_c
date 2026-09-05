param(
    [Parameter(Mandatory = $true)][string]$Version,
    [Parameter(Mandatory = $true)][string]$InstallerUrl,
    [Parameter(Mandatory = $true)][string]$InstallerSha256,
    [string]$Publisher = '46Neon',
    [string]$PackageIdentifier = '46Neon.Mano'
)

if ($InstallerSha256 -notmatch '^[0-9A-Fa-f]{64}$') {
    throw 'InstallerSha256 debe ser un SHA-256 hexadecimal de 64 caracteres.'
}

$root = Join-Path $PSScriptRoot 'winget'
New-Item -ItemType Directory -Force -Path $root | Out-Null

@"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.version.1.9.0.schema.json
PackageIdentifier: $PackageIdentifier
PackageVersion: $Version
DefaultLocale: en-US
ManifestType: version
ManifestVersion: 1.9.0
"@ | Set-Content (Join-Path $root "$PackageIdentifier.yaml")

@"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.defaultLocale.1.9.0.schema.json
PackageIdentifier: $PackageIdentifier
PackageVersion: $Version
PackageLocale: en-US
Publisher: $Publisher
PublisherUrl: https://github.com/46Neon
PackageName: Mano
PackageUrl: https://github.com/46Neon/Mano_versi-n_c
License: MIT
ShortDescription: Mano SST data analysis language
Description: Mano analyzes existing occupational safety and health data to detect statistical patterns and support accident prevention.
ManifestType: defaultLocale
ManifestVersion: 1.9.0
"@ | Set-Content (Join-Path $root "$PackageIdentifier.locale.en-US.yaml")

@"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.installer.1.9.0.schema.json
PackageIdentifier: $PackageIdentifier
PackageVersion: $Version
Installers:
- Architecture: x64
  InstallerType: exe
  InstallerUrl: $InstallerUrl
  InstallerSha256: $InstallerSha256
  InstallerSwitches:
    Silent: /VERYSILENT
    SilentWithProgress: /SILENT
ManifestType: installer
ManifestVersion: 1.9.0
"@ | Set-Content (Join-Path $root "$PackageIdentifier.installer.yaml")

Write-Host "Manifest generado en $root"
