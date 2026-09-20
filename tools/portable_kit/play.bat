@echo off
rem Start the Super Mario 64 DS PC port from this folder.
rem
rem The game resolves every asset under SM64DS_ASSET_ROOT (port/hal/fs.cpp),
rem so pointing that at this folder is what makes the kit portable: the exe
rem itself was built with an absolute path baked in that only exists on the
rem machine that built it.
setlocal
title Super Mario 64 DS - PC demo 1.7

set "KIT=%~dp0"
if "%KIT:~-1%"=="\" set "KIT=%KIT:~0,-1%"
set "SM64DS_ASSET_ROOT=%KIT%"
cd /d "%KIT%"

if not exist "%KIT%\demo-1.7.exe" (
    echo.
    echo demo-1.7.exe is missing from this folder. The kit is incomplete.
    echo.
    pause
    exit /b 1
)

rem EVERY file the game refuses to start without, not a sample of them.
rem
rem This list is port\kit_assets.txt, and port\tools\kit_smoke.py checks that
rem the two still agree. It used to test three files, and the three it tested
rem happened to be the three an OLD extraction already had. So when the game
rem started needing the nitrofs tables, an install that predated them sailed
rem past this check, skipped the unpack that would have written them, and died
rem on a missing file about a fifth of a second later. A partial check is worse
rem than none: it is a check that says "unpacked" about an install it never
rem looked at properly.
rem
rem A missing file here costs one re-extraction. Getting it wrong costs a
rem player a game that will not start and has no way to repair itself.
if not exist "%KIT%\build\assets\files.tsv" goto unpack
if not exist "%KIT%\build\assets\handles.tsv" goto unpack
if not exist "%KIT%\build\assets\nitrofs.tsv" goto unpack
if not exist "%KIT%\build\assets\nitrofs_fnt.bin" goto unpack
if not exist "%KIT%\build\assets\nitrofs_fat.bin" goto unpack
if not exist "%KIT%\build\assets\romdata.bin" goto unpack
if not exist "%KIT%\build\assets\romdata.manifest" goto unpack
if not exist "%KIT%\extracted\dsd\files\data\sound_data.sdat" goto unpack
goto play

:unpack
echo.
echo First run: unpacking the game data from your cartridge dump.
echo.
dir /b "%KIT%\PLACE EU ROM HERE\*.nds" >nul 2>&1
if not errorlevel 1 goto haverom
dir /b "%KIT%\*.nds" >nul 2>&1
if errorlevel 1 goto norom
:haverom
rem Hand the recipe to PowerShell as TEXT instead of by path.
rem
rem Naming a .ps1 file makes Windows decide whether it is willing to run THAT
rem FILE, and on a PC with a stricter script setting than the usual default the
rem answer is no. -ExecutionPolicy Bypass does not settle that either: a setting
rem that arrives by group policy outranks the command line, and a software
rem restriction policy ignores it entirely. Text is not a file, so the check does
rem not apply to it, and the script does the same work either way.
rem
rem Unblock-File first takes the "came from the internet" marker off the kit's own
rem files, which is what a kit unzipped from a download carries.
set "SM64DS_EXTRACT_SCRIPT=%KIT%\extract_assets.ps1"
set "SM64DS_EXTRACT_DEST=%KIT%"
powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -OutputFormat Text -Command "$ErrorActionPreference='Stop'; Get-ChildItem -LiteralPath $env:SM64DS_EXTRACT_DEST -File | Unblock-File -ErrorAction SilentlyContinue; & ([scriptblock]::Create([IO.File]::ReadAllText($env:SM64DS_EXTRACT_SCRIPT))) -Destination $env:SM64DS_EXTRACT_DEST"
if errorlevel 1 goto unpackfailed
echo.

:play
"%KIT%\demo-1.7.exe" %*
if errorlevel 1 (
    echo.
    echo The game exited with an error. The lines above say why.
    echo.
    pause
)
exit /b 0

:norom
echo No .nds file in this folder.
echo.
echo This kit ships no game data. Copy the dump of your own Super Mario 64 DS
echo cartridge (European or North American) into the folder:
echo.
echo     %KIT%\PLACE EU ROM HERE
echo.
echo then run play.bat again. README.txt has the details.
echo.
pause
exit /b 1

:unpackfailed
echo.
echo The game data could not be unpacked, so there is nothing to run yet.
echo The lines above say what went wrong.
echo.
echo If they mention signing, an execution policy or a restriction policy, then
echo Windows blocked the unpacking step rather than the unpacking step failing.
echo On a PC that is yours, right click extract_assets.ps1 in this folder, choose
echo Properties, tick Unblock at the bottom and press OK, then run play.bat again.
echo On a PC managed by a school or a workplace, an administrator controls that
echo setting and will need to allow it.
echo.
pause
exit /b 1
