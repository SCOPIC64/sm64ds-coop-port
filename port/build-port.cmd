@echo off
rem Build the PC port's gate-1 smoke runner: 32-bit MSVC via VS Build Tools,
rem same toolchain-location pattern as the recomp's build scripts.
setlocal
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
if errorlevel 1 exit /b 1
set "CMAKEBIN=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake"
set "PATH=%CMAKEBIN%\CMake\bin;%CMAKEBIN%\Ninja;%PATH%"
rem Fail before configure if a slice gate activates a receiver dropping raw reader.
python "%~dp0tools\closestplayer_guard.py"
if errorlevel 1 exit /b 1
rem Fail before configure if a NEW guessed vtable body got seated past the baseline.
python "%~dp0tools\inferred_stub_guard.py"
if errorlevel 1 exit /b 1
cmake -S "%~dp0." -B "%~dp0..\build\port" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="%CMAKEBIN%\Ninja\ninja.exe" %*
if errorlevel 1 exit /b 1
ninja -C "%~dp0..\build\port"
if errorlevel 1 exit /b 1
rem Fail after link if any /alternatename LHS is also a DEFINED symbol in the
rem map -- a defined LHS defeats the alias silently (the wave-5 R1/R2 class;
rem EyerokD0 and the data_ov075 aliases flip the same way if their overlays
rem land). Post-link by design: the guard needs walk_window.map.
python "%~dp0tools\alternatename_guard.py" --map "%~dp0..\build\port\walk_window.map"
if errorlevel 1 exit /b 1
