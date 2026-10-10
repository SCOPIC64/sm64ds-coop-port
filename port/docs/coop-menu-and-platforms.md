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

Display offers Bob-omb Battlefield, Castle Grounds, Staff Roll,
Whomp's Fortress, Cool Cool Mountain, Jolly Roger Bay, Lethal Lava Land and
Dire Dire Docks. A
hidden child of the same executable renders the actual DS scenery into shared
memory, with audio, input and networking disabled and an isolated temporary
save. A kill-on-close job ties its lifetime to the menu. Staff Roll follows
CoopDX's menu credits behavior: moving cinematic cameras, no player or HUD,
scene fades and a repeating course sequence, while retaining the selected menu
music. It runs the cartridge's 20 course-panorama Kuppa scripts, including
their original camera splines, 204-frame scene timing, course changes and DS
fades. The final panorama returns to the first one before the ending cast scene.
This changes only the isolated renderer's RAM. It does not change ROM files,
player saves or ordinary cutscenes. All Player render paths, including other
cutscene players, are hidden in this renderer. Enemies and scenery keep running
so the original credits behavior remains intact. The renderer is paced at
approximately 30 frames per second; its selected menu music plays independently.

Reference: https://github.com/coop-deluxe/sm64coopdx/blob/main/src/game/level_update.c
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

## Gameplay options and existing mods

The existing `fs_mods.cpp` asset/palette replacement and `stage_mods.cpp` level
editing systems remain available. This adds a built-in optional movement mod;
it does not add Lua scripting or compatibility with CoopDX Lua mods.

* **Player > Movement Mod > SM64** applies N64 walking acceleration, a 32-unit
  target speed, 48-unit positive cap, 0x800 facing-step limit, normal/double/triple
  jump launches and long-jump launch/gravity constants to Mario. It defaults off.
  Cutscenes, other characters, Mega Mario and winged actions keep DS behavior.
  This is a beta movement profile: DS air steering, slope handling, collision
  steps and action transitions remain in use. It is not the complete N64
  moveset. All co-op participants should use matching movement preferences.
* **Camera > SM64 Cam** adds a default Lakitu-style camera with 45-degree
  C-side turns over 16 ticks, 800/1200 zoom, slower yaw follow while moving and
  lateral pan. Q/E, bumpers or right-stick edges step the camera; R/F or vertical
  right-stick movement select zoom; C recenters behind Mario. DS collision
  rays keep the camera clear of walls and floors. Camera motion advances once
  per simulation tick even when the view is loaded more than once. Original
  cutscenes still own their camera. N64 level-specific volumes and all alternate
  N64 camera modes are not reproduced; this is a beta adaptation to DS levels.
* **Display > Object Distance** chooses DS Default, Near (2000), Medium (6000),
  Far (16000) or Unlimited world units. It changes object drawing and preserves
  the native area/lifecycle gates and object simulation. Unlimited removes the
  distance limit, not area visibility or the renderer's projection range.
  Objects beyond their native behavior range can retain their last animation
  pose; raising the draw distance does not rewrite DS object logic.

These choices persist through the same atomic settings writer as the existing
options, preserving unrelated keys and failed-write cache state.

Research references: [N64 camera](https://github.com/n64decomp/sm64/blob/master/src/game/camera.c),
[walking](https://github.com/n64decomp/sm64/blob/master/src/game/mario_actions_moving.c),
[jump launches](https://github.com/n64decomp/sm64/blob/master/src/game/mario.c),
[air steering](https://github.com/n64decomp/sm64/blob/master/src/game/mario_actions_airborne.c),
and [gravity/collision](https://github.com/n64decomp/sm64/blob/master/src/game/mario_step.c).
The helpers implement the measured constants in the DS fixed-point world;
their portable tests do not prove that all DS gameplay behaves like N64 SM64.

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
in the test working directory and returns nonzero if a screenshot could not be written.
Inspect these captures; this check does not enter gameplay or import assets.

With private game assets available, `SM64DS_MENU_AUDIO_SELFTEST=1` checks
non-silent output for every selectable theme and each of the three effects.
`SM64DS_MENU_SELFTEST=1` plus `SM64DS_MENU_BACKDROP_SELFTEST=1` through `8`
waits for ninety rendered scene frames before capturing all menu pages. Always
use an isolated runtime folder and explicit fixture save path for these checks.
`SM64DS_MENU_TOUR_TEST=1` additionally checks a scenery transition.
`SM64DS_MENU_STAFF_ROLL_SELFTEST=1` with `SM64DS_MENU_TOUR_TEST=1` checks all
20 original moving camera sequences, course IDs and the wrap to the first
script. Captures are `staff-roll-00.bmp` through `staff-roll-19.bmp` and a menu
capture. `SM64DS_MENU_CREDITS_FAST=1` removes renderer sleeping for this test;
it leaves original script frame counts and camera paths intact. The portable
`gameplay_models_and_credits_order` test checks independent N64 movement
checkpoints, discrete camera input, object limits and the DS course order.
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
| Linux / SteamOS | Portable component tests; handheld profile; safe native memory reservation | Replace remaining Win32 window/input/audio/socket/process APIs and verify the engine ABI |
| macOS | Portable component tests and POSIX memory backend | Platform APIs plus 64-bit pointer/layout and calling-convention work for Intel and Apple Silicon |
| Android | Native component compile check, reusable multi-touch and page-size-aware memory backend | Native host, ARM ABI, asset import/storage, rendering/audio/networking, lifecycle and physical-device tests |
| iOS | Native component compile check, reusable touch and POSIX memory backend | ARM64 engine portability, UIKit/SDL adapter, sandbox/storage, memory requirements, signing and device testing |

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

## Native memory foundation

The POSIX DS-range reservation now works without assuming
`MAP_FIXED_NOREPLACE` is defined. When the kernel ignores that flag or only a
non-destructive address hint is available, it verifies the returned address
and releases any unwanted mapping. It never uses `MAP_FIXED` to overwrite
another allocation. Mapping spans use the actual system page size, including
16 KB systems. Windows game reservation and write-watch behavior are unchanged.

Portable native-memory tests check allocation, overlapping-request rejection
without losing existing bytes, release/reallocation, overflow and 4/16 KB span
calculation, including the hint-only branch. CI also compiles the memory,
movement/camera and touch components against Android arm64 and iOS arm64 SDKs.
Those compile jobs produce no APK/IPA and establish no on-device gameplay claim.

References: [Linux mmap](https://man7.org/linux/man-pages/man2/mmap.2.html),
[Android page sizes](https://source.android.com/docs/core/architecture/16kb-page-size/16kb).
