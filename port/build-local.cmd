@echo off
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 2
for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSROOT=%%I"
if not defined VSROOT exit /b 2
call "%VSROOT%\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 2
set "CMAKEBIN=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake"
set "PATH=%CMAKEBIN%\CMake\bin;%CMAKEBIN%\Ninja;%PATH%"
ninja -C "%~dp0..\build\port-kit" walk_window resource_pack_probe
if errorlevel 1 exit /b 1
pushd "%~dp0..\build\port-kit"
cl /nologo /EHsc /MT /O2 "%~dp0tests\lockid_probe.cpp" "%~dp0hal\os_lockid.cpp" /Fe:lockid_probe.exe
set "COMPILE_RESULT=%ERRORLEVEL%"
popd
if not "%COMPILE_RESULT%"=="0" exit /b 1
"%~dp0..\build\port-kit\lockid_probe.exe"
exit /b %ERRORLEVEL%
