@echo off
rem Start the Super Mario 64 DS PC port from this folder.
rem The launcher and ROM stay in Desktop\GAMES\SM64 DS COOP; generated game data is
rem kept in %LOCALAPPDATA% so the install folder does not fill with cache files.
setlocal
title Super Mario 64 DS - PC port

set "KIT=%~dp0"
if "%KIT:~-1%"=="\" set "KIT=%KIT:~0,-1%"
set "DATA=%LOCALAPPDATA%\SM64DS"
set "SM64DS_ASSET_ROOT=%DATA%"
rem Start in gameplay. F5 opens the in-game menu when needed.
set "SM64DS_MENU="
cd /d "%KIT%"

for /f "delims=" %%R in ('dir /b /a-d "%KIT%\*.nds" 2^>nul') do (
    if defined ROM (
        set "MULTIPLE=1"
    ) else (
        set "ROM=%KIT%\%%R"
    )
)

if not exist "%KIT%\sm64ds coop.exe" (
    echo.
    echo sm64ds coop.exe is missing from this folder. The kit is incomplete.
    echo.
    pause
    exit /b 1
)

if not defined ROM goto norom
if defined MULTIPLE goto multiplerom

rem Both halves of the AppData asset root have to be there: the catalogs the
rem loader reads first, and the files they point at.
if not exist "%DATA%\build\assets\files.tsv" goto unpack
if not exist "%DATA%\build\assets\handles.tsv" goto unpack
if not exist "%DATA%\extracted\dsd\files\data\sound_data.sdat" goto unpack
goto play

:unpack
echo.
echo First run: unpacking the game data from your cartridge dump.
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%KIT%\tools\extract_assets.ps1" -Destination "%DATA%" -Rom "%ROM%"
if errorlevel 1 goto unpackfailed
echo.

:play
"%KIT%\sm64ds coop.exe" %*
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
echo     %KIT%\sm64ds.nds
echo.
echo so it sits next to play.bat, then run play.bat again. README.txt has the
echo details.
echo.
pause
exit /b 1

:multiplerom
echo More than one .nds file is next to walk_window.exe.
echo Leave only your Super Mario 64 DS dump in:
echo     %KIT%
pause
exit /b 1

:unpackfailed
echo.
echo The game data could not be unpacked, so there is nothing to run yet.
echo The lines above say what went wrong.
echo.
pause
exit /b 1
