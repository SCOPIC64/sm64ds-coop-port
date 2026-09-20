[CmdletBinding()]
param(
    [string] $SourceBuild,
    [string] $InstallFolder
)

$ErrorActionPreference = 'Stop'
if (-not $SourceBuild) {
    $SourceBuild = Join-Path $PSScriptRoot '..\..\build\port-kit'
}
if (-not $InstallFolder) {
    $InstallFolder = Join-Path $env:USERPROFILE 'Desktop\GAMES\SM64 DS COOP'
}

$sourceExe = Join-Path $SourceBuild 'walk_window.exe'
if (-not (Test-Path -LiteralPath $sourceExe -PathType Leaf)) {
    throw "The installer needs a built executable at $sourceExe. Build walk_window first."
}

$kitScript = Join-Path $PSScriptRoot 'package_kit.ps1'
& $kitScript -Output $InstallFolder -SkipBuild
if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
    throw "The package step failed with exit code $LASTEXITCODE."
}

Write-Host "Installed SM64 DS COOP to $InstallFolder" -ForegroundColor Green