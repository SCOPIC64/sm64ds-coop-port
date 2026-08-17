@echo off
rem gen_disasm.cmd  <build-dir>  [out-dir]
rem
rem Fills abicheck's input directory with the disassembly of every hal object
rem in the build, in the ONE format abicheck can parse: /disasm:nobytes. Plain
rem /disasm interleaves the encoded bytes between the address and the mnemonic,
rem the vtable-store pattern never matches, and abicheck reports "0 vtable slot
rem fills". abicheck REFUSES a zero-fill run rather than passing it, but
rem generating the wrong format here is the mistake that causes it, so this
rem script never emits anything else.
rem
rem OUT-DIR DEFAULTS INSIDE THE BUILD DIRECTORY, not to a fixed absolute path.
rem The version of this recovered from branch port-abi-sweep wrote to a shared
rem C:\tmp\sm64ds-abisweep\_disasm, so two lanes running the suite at once
rem would each have measured the other's build and both would have read green.
rem The disassembly now travels with the build it describes.
rem
rem IT ENUMERATES THE hal DIRECTORIES RATHER THAN WALKING THE BUILD TREE. A
rem `for /r` over the whole build directory visits every one of the port's
rem fourteen thousand objects and filters each through a piped findstr; that
rem took many minutes here and grew cmd.exe past two gigabytes. The objects are
rem always at <build>\CMakeFiles\<target>.dir\hal\*.obj, so this walks that
rem shape directly. If a future generator moves them, FOUND stays 0 and the
rem script exits 3 rather than quietly disassembling nothing.
rem
rem dumpbin comes from the same VS Build Tools the port builds with; the caller
rem is expected to have run vcvars32 already (build-port.cmd does), but this
rem re-invokes it defensively so the script also works when run by hand.
setlocal enabledelayedexpansion
where dumpbin >nul 2>nul
if errorlevel 1 call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" >nul
set "OBJDIR=%~1"
if "%OBJDIR%"=="" (
  echo usage: gen_disasm.cmd ^<build-dir^> [out-dir]
  exit /b 2
)
set "OUT=%~2"
if "%OUT%"=="" set "OUT=%OBJDIR%\_abicheck_disasm"
if not exist "%OUT%" mkdir "%OUT%"
del /q "%OUT%\*.txt" 2>nul
set FOUND=0
for /d %%d in ("%OBJDIR%\CMakeFiles\*.dir") do (
  if exist "%%d\hal\*.obj" (
    for %%f in ("%%d\hal\*.obj") do (
      set /a FOUND+=1
      dumpbin /nologo /disasm:nobytes "%%f" > "%OUT%\%%~nxf.txt"
    )
  )
)
if "!FOUND!"=="0" (
  echo gen_disasm: no hal .obj files under "%OBJDIR%\CMakeFiles\*.dir\hal"
  echo gen_disasm: build the port first, or point this at the right build dir
  exit /b 3
)
echo gen_disasm: wrote /disasm:nobytes for !FOUND! hal objects to "%OUT%"
exit /b 0
