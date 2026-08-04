@echo off
rem Start the Super Mario 64 DS PC port from this folder.
rem
rem The game resolves every asset under SM64DS_ASSET_ROOT (port/hal/fs.cpp),
rem so pointing that at this folder is what makes the kit portable: the exe
rem itself was built with an absolute path baked in that only exists on the
rem machine that built it.
setlocal
title Super Mario 64 DS - PC port

set "KIT=%~dp0"
if "%KIT:~-1%"=="\" set "KIT=%KIT:~0,-1%"
set "SM64DS_ASSET_ROOT=%KIT%"
cd /d "%KIT%"

if not exist "%KIT%\walk_window.exe" (
    echo.
    echo walk_window.exe is missing from this folder. The kit is incomplete.
    echo.
    pause
    exit /b 1
)

rem Both halves of the asset root have to be there: the catalogs the loader
rem reads first, and the files they point at.
if not exist "%KIT%\build\assets\files.tsv" goto unpack
if not exist "%KIT%\build\assets\handles.tsv" goto unpack
if not exist "%KIT%\extracted\dsd\files\data\sound_data.sdat" goto unpack
goto play

:unpack
echo.
echo First run: unpacking the game data from your cartridge dump.
echo.
dir /b "%KIT%\*.nds" >nul 2>&1
if errorlevel 1 goto norom
powershell -NoProfile -ExecutionPolicy Bypass -File "%KIT%\extract_assets.ps1"
if errorlevel 1 goto unpackfailed
echo.

:play
"%KIT%\walk_window.exe" %*
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
echo cartridge into:
echo.
echo     %KIT%
echo.
echo so it sits next to play.bat, then run play.bat again. README.txt has the
echo details.
echo.
pause
exit /b 1

:unpackfailed
echo.
echo The game data could not be unpacked, so there is nothing to run yet.
echo The lines above say what went wrong.
echo.
pause
exit /b 1
