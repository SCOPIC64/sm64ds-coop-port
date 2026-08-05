<#
.SYNOPSIS
    Assemble the portable kit folder that gets zipped and sent out.

.DESCRIPTION
    Builds walk_window against the STATIC CRT into its own binary directory
    and copies it, the launcher, the extractor and the README into one folder.
    The static link is the point: the ordinary build/port/walk_window.exe needs
    the Visual C++ 2015-2022 x86 redistributable installed, which is not a
    thing to ask of someone who just wants to double-click play.bat.

    The kit that comes out holds no game data whatsoever. It is code, scripts
    and text. The person receiving it supplies their own cartridge dump and
    extract_assets.ps1 unpacks it on their machine.

    This does not zip anything. It prints the folder; zip it yourself.

.PARAMETER Output
    Where to assemble. Defaults to build\kit\SM64DS-PC in the repository.

.PARAMETER SkipBuild
    Reuse whatever is already in build\port-kit instead of running cmake.

.EXAMPLE
    .\package_kit.ps1
#>
[CmdletBinding()]
param(
    [string] $Output,
    [switch] $SkipBuild
)

$ErrorActionPreference = 'Stop'

$here = if ($PSScriptRoot) { $PSScriptRoot } else { (Get-Location).Path }
$repo = (Resolve-Path (Join-Path $here '..\..')).Path
if (-not $Output) { $Output = Join-Path $repo 'build\kit\SM64DS-PC-demo-1.7' }

$buildDir = Join-Path $repo 'build\port-kit'
$exe = Join-Path $buildDir 'walk_window.exe'

if (-not $SkipBuild) {
    # Same toolchain location pattern as port\build-port.cmd, plus the one
    # extra switch. A separate binary directory keeps the normal build's
    # object files from being thrown away every time the kit is packaged.
    $vsRoot = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\2022\BuildTools'
    $vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
    $cmakeRoot = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake'
    if (-not (Test-Path $vcvars)) {
        throw "Visual Studio 2022 Build Tools not found at $vsRoot"
    }
    $script = @"
@echo off
call "$vcvars" >nul || exit /b 1
set "PATH=$cmakeRoot\CMake\bin;$cmakeRoot\Ninja;%PATH%"
cmake -S "$repo\port" -B "$buildDir" -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_MAKE_PROGRAM="$cmakeRoot\Ninja\ninja.exe" -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded || exit /b 1
ninja -C "$buildDir" walk_window
"@
    $tmp = Join-Path ([IO.Path]::GetTempPath()) ("kitbuild_{0}.cmd" -f [guid]::NewGuid().ToString('N'))
    Set-Content -LiteralPath $tmp -Value $script -Encoding ASCII
    try {
        Write-Host "Building walk_window with the static runtime"
        & cmd /c $tmp
        if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
    } finally {
        Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue
    }
}

if (-not (Test-Path $exe)) { throw "no walk_window.exe at $exe" }

# A kit whose exe still wants MSVCP140.dll would fail on the target machine
# with a dialog box and no explanation, so check rather than hope.
$dumpbin = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
if ($dumpbin) {
    $needs = & $dumpbin /dependents $exe | Select-String -Pattern 'MSVCP140|VCRUNTIME140'
    if ($needs) {
        throw "walk_window.exe still imports the VC++ redistributable: $needs"
    }
}

Write-Host "Assembling $Output"
$kitFiles = 'demo-1.7.exe', 'play.bat', 'extract_assets.ps1', 'README.txt',
            'PLACE EU ROM HERE'

# Never clear the folder out: the obvious way to test a kit is to drop a
# cartridge dump into it and run it, and this must not be the thing that
# deletes it. Refuse an untidy folder instead, because the promise the kit
# makes is that the zip holds nothing but these four files.
if (Test-Path $Output) {
    $stray = Get-ChildItem $Output -Force | Where-Object { $_.Name -notin $kitFiles }
    if ($stray) {
        Write-Host ""
        $stray | ForEach-Object { Write-Host "    $($_.Name)" }
        throw ("$Output already holds the above, which would ship with the kit. " +
               "Clear them out yourself, or pass -Output <a fresh folder>.")
    }
}
[void][IO.Directory]::CreateDirectory($Output)

Copy-Item $exe (Join-Path $Output 'demo-1.7.exe') -Force
# The drop folder ships empty except for one line of instructions, so the
# recipient sees where the dump goes before reading anything.
$dropDir = Join-Path $Output 'PLACE EU ROM HERE'
[void][IO.Directory]::CreateDirectory($dropDir)
Set-Content -LiteralPath (Join-Path $dropDir 'put your sm64ds nds file in this folder.txt') `
    -Value 'Copy the .nds dump of your own Super Mario 64 DS cartridge into this folder, then run play.bat.' `
    -Encoding ASCII
foreach ($name in 'play.bat', 'extract_assets.ps1', 'README.txt') {
    Copy-Item (Join-Path $here $name) (Join-Path $Output $name) -Force
}

Write-Host ""
Get-ChildItem $Output | ForEach-Object {
    "    {0,-20} {1,10:N0} bytes" -f $_.Name, $_.Length
}
Write-Host ""
Write-Host "Kit ready:" -ForegroundColor Green
Write-Host "    $Output"
Write-Host ""
Write-Host "Zip that folder and send it. The person on the other end drops their"
Write-Host "own .nds dump into PLACE EU ROM HERE and double-clicks play.bat."
