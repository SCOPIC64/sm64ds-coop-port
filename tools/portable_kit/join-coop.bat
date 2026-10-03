@echo off
setlocal
powershell -NoProfile -ExecutionPolicy RemoteSigned -File "%~dp0coop.ps1" -Mode join
set "RESULT=%ERRORLEVEL%"
if not "%RESULT%"=="0" pause
exit /b %RESULT%
