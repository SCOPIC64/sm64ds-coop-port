@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install_sm64ds_coop.ps1"
if errorlevel 1 (
    echo.
    echo Installation could not finish. The message above explains why.
    pause
    exit /b 1
)
echo.
echo SM64 DS COOP was installed successfully.
pause