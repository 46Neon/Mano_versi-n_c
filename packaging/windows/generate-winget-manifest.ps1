[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][ValidatePattern('^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$')][string]$Version,
    [Parameter(Mandatory = $true)][ValidatePattern('^https://')][string]$InstallerUrl,
    [Parameter(Mandatory = $true)][ValidatePattern('^[0-9A-Fa-f]{64}$')][string]$InstallerSha256,
    [ValidateSet('portable', 'exe')][string]$InstallerType = 'portable',
    [string]$Publisher = '46Neon',
    [string]$PackageIdentifier = '46Neon.Milena',
    [string]$PackageName = 'Milena',
    [string]$PublisherUrl = 'https://github.com/46Neon',
    [string]$PackageUrl = 'https://github.com/46Neon/Milena',
    [string[]]$Commands = @('milena'),
    [string]$SilentSwitch,
    [string]$SilentWithProgressSwitch
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ($InstallerType -eq 'portable' -and $Commands.Count -eq 0) {
    throw 'Un paquete portable debe declarar al menos un comando.'
}
if ($InstallerType -eq 'exe' -and [string]::IsNullOrWhiteSpace($SilentSwitch)) {
    throw 'Un instalador exe requiere -SilentSwitch real; no se deben inventar switches.'
}
if ($PackageIdentifier -notmatch '^[A-Za-z0-9][A-Za-z0-9.-]+$') {
    throw 'PackageIdentifier contiene caracteres no válidos.'
}

$root = Join-Path $PSScriptRoot 'winget'
New-Item -ItemType Directory -Force -Path $root | Out-Null
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)

$versionManifest = @"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.version.1.9.0.schema.json
PackageIdentifier: $PackageIdentifier
PackageVersion: $Version
DefaultLocale: en-US
ManifestType: version
ManifestVersion: 1.9.0
"@

$localeManifest = @"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.defaultLocale.1.9.0.schema.json
PackageIdentifier: $PackageIdentifier
PackageVersion: $Version
PackageLocale: en-US
Publisher: $Publisher
PublisherUrl: $PublisherUrl
PackageName: $PackageName
PackageUrl: $PackageUrl
License: MIT
ShortDescription: Milena SST data analysis language
Description: Milena analyzes occupational safety and health data to detect statistical patterns and support accident prevention.
ManifestType: defaultLocale
ManifestVersion: 1.9.0
"@

$installerLines = @(
    '# yaml-language-server: `$schema=https://aka.ms/winget-manifest.installer.1.9.0.schema.json',
    "PackageIdentifier: $PackageIdentifier",
    "PackageVersion: $Version",
    'Installers:',
    '- Architecture: x64',
    "  InstallerType: $InstallerType",
    "  InstallerUrl: $InstallerUrl",
    "  InstallerSha256: $($InstallerSha256.ToUpperInvariant())"
)
if ($InstallerType -eq 'portable') {
    $installerLines += '  Commands:'
    foreach ($command in $Commands) { $installerLines += "  - $command" }
} else {
    $installerLines += '  InstallerSwitches:'
    $installerLines += "    Silent: $SilentSwitch"
    if (-not [string]::IsNullOrWhiteSpace($SilentWithProgressSwitch)) {
        $installerLines += "    SilentWithProgress: $SilentWithProgressSwitch"
    }
}
$installerLines += @('ManifestType: installer', 'ManifestVersion: 1.9.0')

[System.IO.File]::WriteAllText((Join-Path $root "$PackageIdentifier.yaml"), $versionManifest, $utf8NoBom)
[System.IO.File]::WriteAllText((Join-Path $root "$PackageIdentifier.locale.en-US.yaml"), $localeManifest, $utf8NoBom)
[System.IO.File]::WriteAllLines((Join-Path $root "$PackageIdentifier.installer.yaml"), $installerLines, $utf8NoBom)

Write-Host "Manifest WinGet generado en $root"
Write-Host "Tipo: $InstallerType | SHA-256: $($InstallerSha256.ToUpperInvariant())"
