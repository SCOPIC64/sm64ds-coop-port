# Co-op menu and platform status

This change targets `codex/coop-release-054` (base `f3657f258c2fa246e7a36958f9352fd5dfd1cc22`).
The repository's `coop-port` branch is a different engine base. Do not mix its
decomp sources, manifests, or runtime tables with the 0.5.4 release.

## Windows integration

Ordinary launches open a CoopDX-inspired menu inside the existing game process
and window. Play selects save A/B/C. The existing retail save-loading routines
run behind a loading screen; their memory layout and cartridge data remain
unchanged. Host/Join use the same LAN transport and separate co-op saves as the
0.5.4 preview's batch launchers. Options save through the existing settings
writer and preserve unrelated keys. Keyboard, mouse, and gamepad navigate the
front end. F5 retains the development menu during gameplay.

The executable discovers its asset root beside itself when no explicit root was
provided. Developer scene/level requests and selftests bypass the front end;
`SM64DS_CLASSIC_MENU=1` also restores the old boot for comparison.

`PORT_ROM_CLEAN=ON` embeds the extraction script and its matching offsets/hashes
as Windows resources. These contain no Nintendo assets. On first Play/Host/Join,
the game runs that importer using Windows PowerShell, respecting the machine's
script policy. Its output goes to `extraction.log`. Closing the window stops
the importer; a later launch checks for missing required assets. The temporary
script and only metadata files created by that attempt are removed afterwards.
Put your own supported `.nds` beside the executable or in the existing ROM drop
folder. Extracted assets, settings and saves remain external writable files.

Use `tools/portable_kit/package_single_exe.ps1 -Executable <built-exe> -Output
<empty-folder>` to package a single program file. The legacy packaging script is
retained for older builds. No batch launcher is needed for the new executable.

## Portable UI/input checks

```
cmake -S port/frontend -B build/frontend
cmake --build build/frontend --config Debug
ctest --test-dir build/frontend -C Debug --output-on-failure
```

These tests cover menu routes, malformed addresses/ports, save selection, option
bounds, render clipping at several aspect ratios, simultaneous touches, shared
button ownership, slide-off release, cancellation and rotation. They do not boot
the game or establish multiplayer or device compatibility. The separate CI
matrix checks this component on Windows, Linux and macOS, with `HANDHELD` both
off and on. It publishes no game binaries.

Windows/MSVC also tests the actual options writer in separate processes: native
timing and 144 Hz survive a restart, boolean options keep their values, unrelated
settings survive, and a locked-file save leaves the cached settings intact.
The frame-rate menu uses the engine's native-timing sentinel (0), or presentation
rates from 60 to 240 Hz; it does not change the game clock.

## SteamOS

CoopDX's build workflow builds Linux normally and SteamOS from the same code with
`HANDHELD=1`, producing separate archives:
https://github.com/coop-deluxe/sm64coopdx/blob/main/.github/workflows/build-coop.yaml

This front end follows that build-profile distinction. It is not a second OS
implementation. Full native Linux/SteamOS releases are not available from this
change; the engine platform layer still needs porting.

## Remaining engine work — no non-Windows game releases yet

| Platform | UI/input foundation | Full engine blocker |
| --- | --- | --- |
| Windows x86 | Integrated into existing host; runtime verification required | Complete MSVC build, clean-folder extraction, three save slots, two-client co-op and return-to-menu testing |
| Linux / SteamOS | Portable model/render/input tests; handheld profile | Replace Win32 window, input, audio, sockets, virtual-memory and process APIs; reproduce and verify the engine ABI |
| macOS | Portable component tests | Platform APIs plus 64-bit pointer/layout and calling-convention work for Intel and Apple Silicon |
| Android | Multi-touch control state with stable finger IDs | Native host, ARM ABI, asset import/storage, rendering/audio/networking, lifecycle and physical-device tests |
| iOS | Same reusable touch state | ARM64 engine portability, UIKit/SDL adapter, sandbox/storage, memory requirements, signing and device testing |

The current port depends on four-byte pointers, MSVC-specific forwarding,
pointer-to-member representations, and fixed DS address mappings. Changing a
CMake OS name or packaging a Windows binary as an APK cannot solve those issues.

`coop_touch.h` is the reusable input component, **not an Android adapter**. A
mobile host must forward down/move/up events with their original finger IDs,
draw visible controls corresponding to its normalized hit regions, merge its
axes/buttons into the game's input, forward menu taps separately, request the
system keyboard for address fields, and call `cancel()` on focus loss or pause
and `resize()` after rotation or safe-area changes. Those connections are not
implemented by this change. No Android/iOS gameplay or touchscreen-on-device
claim is made.

Before releasing any platform, test fresh extraction, existing/new save slots,
all menu routes, resize/fullscreen, focus changes, input devices, successful and
failed joins, disconnects, level transitions and save persistence. Mobile also
needs simultaneous movement/jump/attack, touch cancellation, orientation, safe
areas, background/resume and keyboard tests on real devices. Passing component
tests is not a promise of a bug-free game.
