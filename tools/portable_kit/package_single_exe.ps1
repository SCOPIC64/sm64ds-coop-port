<# Package the ROM-clean game with its built-in menu and extraction resources.
   Requires a tested PORT_ROM_CLEAN build. Never package a played-in folder. #>
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $Executable).Path
$bytes = [IO.File]::ReadAllBytes($source)
$text = [Text.Encoding]::ASCII.GetString($bytes)
if ($text.IndexOf('build/assets/romdata.bin') -lt 0 -or $text.IndexOf('[coop-menu]') -lt 0) {
    throw 'Expected the ROM-clean game with the integrated co-op menu.'
}
if ($text.IndexOf('# romdata-recipe v1') -lt 0 -or $text.IndexOf('param(') -lt 0) {
    throw 'The executable is missing its first-run extraction resources.'
}
if (Test-Path -LiteralPath $Output) {
    if (@(Get-ChildItem -LiteralPath $Output -Force).Count -ne 0) {
        throw 'Choose an empty output directory. Existing assets and saves are never removed.'
    }
} else {
    [void][IO.Directory]::CreateDirectory($Output)
}
Copy-Item -LiteralPath $source -Destination (Join-Path $Output 'sm64ds coop.exe')
Write-Output 'Packaged one executable. No cartridge, extracted assets, settings or saves included.'
Write-Output 'The player puts their own .nds beside the executable and selects Play.'
Write-Output 'Packaging checks are not a substitute for a clean-folder runtime test.'
