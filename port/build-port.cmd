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
rem Fail before configure if the closure prober's selftest breaks: the probe
rem sizes slice walls and predicts collisions, and a broken prober lies
rem quietly. There is no port CI; this block is where loudness lives.
python "%~dp0tools\closure.py" --selftest
if errorlevel 1 exit /b 1
rem Fail before configure if facegen's selftest breaks: generated faces get
rem wired by slices, and a generator that stops refusing the judgment rows
rem is a silent hazard, not a convenience.
python "%~dp0tools\facegen.py" --selftest
if errorlevel 1 exit /b 1
rem Fail before configure if mapdiff's selftest breaks: reviews and delta-0
rem claims read their decomposition off it, and a differ that miscounts or
rem stops refusing a truncated map turns a review into an eyeball again.
python "%~dp0tools\mapdiff.py" --selftest
if errorlevel 1 exit /b 1
rem Fail before configure if vtablerows' selftest breaks: the minigame
rem fan-out lanes read their override/marker/nosrc census off it, and a
rem reader that miscounts a marker row skips a ROM adjudication silently.
python "%~dp0tools\vtablerows.py" --selftest
if errorlevel 1 exit /b 1
rem Fail before configure if the alternatename guard's scoping fixture breaks.
rem The guard decides what counts as a linker input, and it used to read lane
rem prose as one: a quoted directive in a .txt was a build input, so deleting a
rem real alias left the quote of it failing the build. The fixture pins that
rem scope. The guard's own map check still runs post-link, below.
python "%~dp0tools\alternatename_guard.py" --selftest
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
