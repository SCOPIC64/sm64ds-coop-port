<#
.SYNOPSIS
    Assemble the portable kit folder that gets zipped and sent out.

.DESCRIPTION
    Builds the game against the STATIC CRT into its own binary directory and
    copies the main executable and launcher files into the kit root. Optional
    tools and alternate executables go under tools\.
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
if (-not $Output) {
    $Output = Join-Path $env:USERPROFILE 'Desktop\GAMES\SM64 DS COOP'
}

$buildDir = Join-Path $repo 'build\port-kit'
$exe = Join-Path $buildDir 'walk_window.exe'
$packagedExe = Join-Path $Output 'sm64ds coop.exe'
$packagedHires = Join-Path $Output 'tools\sm64ds coop hires.exe'

if (-not $SkipBuild) {
    # Same toolchain location pattern as port\build-port.cmd, plus the one
    # extra switch. A separate binary directory keeps the normal build's
    # object files from being thrown away every time the kit is packaged.
    $vsCandidates = @(
        (Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\18\Community'),
        (Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\2022\Community'),
        (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\2022\BuildTools')
    )
    $vsRoot = $vsCandidates |
        Where-Object { Test-Path (Join-Path $_ 'VC\Auxiliary\Build\vcvars32.bat') } |
        Select-Object -First 1
    if (-not $vsRoot) {
                throw @"
Visual Studio's 32-bit C++ build tools are missing.
Install Visual Studio 2022 or 2026 Build Tools with the workload:
    Desktop development with C++
and the individual component:
    MSVC v143/v145 - VS C++ x86/x64 build tools
Then run this script again.
"@
    }
    $msvcHeader = Get-ChildItem (Join-Path $vsRoot 'VC\Tools\MSVC') -Filter 'cstdio' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    $ucrtHeader = Get-ChildItem (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include') -Filter 'stdio.h' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $msvcHeader -or -not $ucrtHeader) {
        throw @"
Visual Studio's C++ headers or Windows SDK C runtime headers are missing.
In Visual Studio Installer, select Desktop development with C++ and enable:
  MSVC v145 - VS 2026 C++ x86/x64 build tools
  Windows 11 SDK
Then click Modify and run this script again.
"@
    }
    $vcvars = Join-Path $vsRoot 'VC\Auxiliary\Build\vcvars32.bat'
    $cmakeRoot = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake'
    $vcTools = Get-ChildItem (Join-Path $vsRoot 'VC\Tools\MSVC') -Directory |
        Sort-Object Name -Descending | Select-Object -First 1
    $kitsRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10'
    $sdkVersion = Get-ChildItem (Join-Path $kitsRoot 'Include') -Directory |
        Sort-Object Name -Descending | Select-Object -First 1
    $sdkInclude = Join-Path $kitsRoot "Include\$($sdkVersion.Name)"
    $sdkLib = Join-Path $kitsRoot "Lib\$($sdkVersion.Name)"
    $sdkBin = Join-Path $kitsRoot "bin\$($sdkVersion.Name)\x86"
    $script = @"
@echo off
call "$vcvars" >nul || exit /b 1
set "PATH=$cmakeRoot\CMake\bin;$cmakeRoot\Ninja;$sdkBin;%PATH%"
set "INCLUDE=$($vcTools.FullName)\include;$sdkInclude\ucrt;$sdkInclude\shared;$sdkInclude\um;%INCLUDE%"
set "LIB=$($vcTools.FullName)\lib\x86;$sdkLib\ucrt\x86;$sdkLib\um\x86;%LIB%"
cmake -S "$repo\port" -B "$buildDir" -G Ninja -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_MAKE_PROGRAM="$cmakeRoot\Ninja\ninja.exe" -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded || exit /b 1
ninja -C "$buildDir" walk_window walk_window_hires
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
$kitFiles = 'sm64ds coop.exe', 'play.bat', 'README.txt', 'LUA_MODDING.md',
    'logo.bmp', 'sm64ds.nds', 'tools', 'mods', 'playlog'

# Never clear the folder out: the obvious way to test a kit is to drop a
# cartridge dump into it and run it, and this must not be the thing that
# deletes it. Refuse an untidy folder instead, because the promise the kit
# makes is that the zip holds nothing but these four files.
if (Test-Path $Output) {
    $stray = Get-ChildItem $Output -Force | Where-Object {
        $_.Name -notin $kitFiles -and $_.Extension -ne '.nds' -and
        $_.Name -ne 'playlog'
    }
    if ($stray) {
        Write-Host ""
        $stray | ForEach-Object { Write-Host "    $($_.Name)" }
        throw ("$Output already holds the above, which would ship with the kit. " +
               "Clear them out yourself, or pass -Output <a fresh folder>.")
    }
}
[void][IO.Directory]::CreateDirectory($Output)

Copy-Item $exe (Join-Path $Output 'sm64ds coop.exe') -Force
foreach ($name in 'play.bat', 'README.txt') {
    Copy-Item (Join-Path $here $name) (Join-Path $Output $name) -Force
}
$logo = Join-Path $repo 'port\assets\logo.bmp'
if (Test-Path $logo) {
    Copy-Item $logo (Join-Path $Output 'logo.bmp') -Force
}
[void][IO.Directory]::CreateDirectory((Join-Path $Output 'tools'))
Copy-Item (Join-Path $here 'extract_assets.ps1') (Join-Path $Output 'tools\extract_assets.ps1') -Force
$modSource = Join-Path $repo 'mods'
if (Test-Path $modSource) {
    $modTarget = Join-Path $Output 'mods'
    [void][IO.Directory]::CreateDirectory($modTarget)
    Get-ChildItem $modSource -Directory | ForEach-Object {
        $target = Join-Path $modTarget $_.Name
        [void][IO.Directory]::CreateDirectory($target)
        $main = Join-Path $_.FullName 'main.lua'
        if (Test-Path $main) { Copy-Item $main (Join-Path $target 'main.lua') -Force }
    }
}
$hires = Join-Path $buildDir 'walk_window_hires.exe'
if (Test-Path $hires) {
    Copy-Item $hires (Join-Path $Output 'tools\sm64ds coop hires.exe') -Force
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
Write-Host "own .nds dump in next to play.bat and double-clicks play.bat."
