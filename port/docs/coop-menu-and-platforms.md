# Co-op menu and platform status

This change targets `codex/coop-release-054` (base `f3657f258c2fa246e7a36958f9352fd5dfd1cc22`).
The repository's `coop-port` branch is a different engine base. Do not mix its
decomp sources, manifests, or runtime tables with the 0.5.4 release.

## Windows integration

Ordinary launches open the menu inside the existing game process and window.
Retro shadowed text, a hand cursor and translucent panels follow the requested
SM64Plus presentation; Host, Join, Options and Quit retain the CoopDX arrangement.
Options groups supported settings under Player, Camera, Controls, Display,
Sound and Misc. Keyboard, mouse and gamepad share the same navigation model.
F5 retains the development menu during gameplay.

Host displays three real save cards with star counts, new-game status and
damaged-save warnings. Both primary and mirrored EEPROM records are checked;
this preview never edits or repairs the save. The fourth EEPROM record belongs
to minigames, so it is not exposed as another adventure slot. The chosen file
is passed to the game's existing title/save-loading routines for solo and host.
Joining keeps its separate co-op save. Multiplayer save loading remains a
runtime verification item before release.

Sound offers Random DS, Off, or an individual full-length theme from the player's
DS sound archive, including Bob-omb Battlefield and Staff Roll. Boss fights,
short stingers and temporary power-up cues are excluded. Random selects on
startup and after a non-looping track finishes; Next Random Song changes it
immediately. Cursor, confirm and cancel effects are separately switchable.
Built-in music and effects use the existing native ARM7 sequencer and mixer; no
Nintendo audio files are downloaded or packaged. Sound is reset before handing off to
the game's ARM9 audio initialization.

Add Custom Music opens a file picker for MP3, FLAC or WAV. A validated copy goes
into `menu/music` beside the executable. Select one song to loop it, Shuffle
Custom to shuffle those files, or Shuffle All to mix them with the DS themes.
Next Song skips immediately. Shuffle avoids repeating the previous item when
there are at least two available. Refresh Custom Files scans both libraries.
Custom music streams through the existing menu mixer at 32768 Hz, stereo PCM;
the decoder does not open another audio device. The menu gains apply to both
sources. The decoder is vendored with its license and pinned provenance.

Display offers Bob-omb Battlefield, Castle Grounds, Staff Roll Scenery,
Whomp's Fortress, Cool Cool Mountain, Jolly Roger Bay, Lethal Lava Land and
Dire Dire Docks. A
hidden child of the same executable renders the actual DS scenery into shared
memory, with audio, input and networking disabled and an isolated temporary
save. A kill-on-close job ties its lifetime to the menu. The scenery tour cycles
seven views every 20 seconds; it is not the cartridge's credits
sequence. The renderer is throttled to approximately 30 frames per second.
A gradient keeps the menu usable while the scene loads or assets are missing.

Add Custom Background accepts PNG, JPEG or BMP and copies a validated image to
`menu/backgrounds`. Images are cropped to fill the menu and decoded only when
the selection changes. Custom pictures work before game data is imported.
Libraries expose at most 256 files each. Imports never overwrite an existing
file, accept music up to 512 MiB and images up to 32 MiB, and reject undecodable
files. Custom selections persist by filename, so sorting a new file into the
library does not select a different song or picture after restart. A missing
selected file falls back to a built-in choice. Removing files and pressing
Refresh updates the available choices. Back saves settings.

Back from a settings category saves and returns to Options; Back from Options
saves and returns home. The atomic writer preserves unrelated settings and
keeps cached values intact after a failed write. Music, background and menu
effects also persist across a restart.

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

After linking Windows, copy the executable into an isolated empty directory and
run it from a different working directory with `SM64DS_MENU_SELFTEST=1`. This
exercises the actual window, temporary menu framebuffer and executable-directory
default without a ROM. It writes `coop-menu-00.bmp` through `coop-menu-11.bmp`
beside the executable and returns nonzero if a screenshot could not be written.
Inspect these captures; this check does not enter gameplay or import assets.

With private game assets available, `SM64DS_MENU_AUDIO_SELFTEST=1` checks
non-silent output for every selectable theme and each of the three effects.
`SM64DS_MENU_SELFTEST=1` plus `SM64DS_MENU_BACKDROP_SELFTEST=1` through `8`
waits for ninety rendered scene frames before capturing all menu pages. Always
use an isolated runtime folder and explicit fixture save path for these checks.
`SM64DS_MENU_TOUR_TEST=1` additionally checks a scenery transition.
`SM64DS_MENU_CUSTOM_MEDIA_SELFTEST=1` checks the isolated custom libraries and
captures a custom-background menu. Portable decoder tests use synthetic tones
and generated color images for every supported format, with no cartridge data.

These tests cover menu routes, malformed addresses/ports, save selection, option
bounds, render clipping at several aspect ratios, simultaneous touches, shared
button ownership, slide-off release, cancellation and rotation. They do not boot
the game or establish multiplayer or device compatibility. The separate CI
matrix checks this component on Windows, Linux and macOS, with `HANDHELD` both
off and on. It publishes no game binaries.

Windows/MSVC also tests the actual options writer in separate processes: native
timing and 144 Hz survive a restart, boolean options keep their values, unrelated
settings survive, music/background/effects persist, and a locked-file save
leaves the cached settings intact. Save-preview tests use a golden EEPROM
record and cover mirror fallback, corrupted checksums and invalid file sizes.
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
| Windows x86 | Integrated and locally boot tested | Fresh-folder import, adventure save-loading runtime, two-client co-op and return-to-menu testing |
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
