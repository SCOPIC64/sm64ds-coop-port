# SM64DS Co-op 0.5.4 Preview 1

This is an experimental preview, not a stable or bug-free release.

## Source Base

Based on tangosdev/sm64ds-decomp `release/game-0.5.4`, commit
`7366f2568`. Its decompilation and PC support sources are kept together.
The newer upstream `main` includes source migrations that are not compatible
with this host build; it has not been substituted piecemeal. The fork's Lua
resource-pack support is carried forward with additional validation.
No retail-matching `src/` or shared `include/` files were changed by these fixes.

## Fixes

- Correct the Windows calling convention for door screen wipes.
- Recognize fade-setter receivers without relying on a compiler's temporary
  register values, preventing a crash during level changes.
- Initialize OS lock IDs before early allocation and never reseed live IDs.
- In ROM-free builds, register the filesystem after the archive name is loaded.
- Start the joining PC's local cartridge reader, which the DS download-play
  path normally skips, so both peers can load their local assets.
- Reject overflowing character IDs, non-finite hitbox sizes and duplicate
  texture hashes; fix Lua allocation accounting and exception cleanup.
- Preserve launch error codes and refuse packaging a ROM-filled drop folder.

## Play

Run `play.bat` for solo play. `host-coop.bat` and `join-coop.bat` start a
two-player, direct-connect party on a trusted LAN, at the castle grounds.
Joining requires the host's IPv4 address. UDP 51765 is used; no firewall or
router settings are changed automatically. No public room service is included.
Both peers require the same build and locally extracted cartridge data.

Defaults: analog movement/camera, widescreen, smooth 60 FPS presentation,
name tags and rollback networking. Voice and mouse capture are off.
This is not SM64CoopDX and does not interoperate with it.
Keep this preview separate from old installs and do not migrate save states.

## Verification

Windows x86 Release build, static CRT, `PORT_ROM_CLEAN=ON`, MSVC 19.51:

- Seven gameplay checks pass: grounds, basement door, locked front door,
  front door after key collection, interior exit, repeated level changes,
  and a course. Each requires clean exit and a nonblank rendered image;
  transition cases require actual level-change evidence.
- Two-player loopback and direct-connect tests pass, 600 frames per peer:
  joining, network-mode agreement, body movement, collision/ghost distinction,
  same-frame replay and zero unrecoverable rewinds.
- Nine resource-pack tests pass, including memory/instruction limits.
- Lock-ID test passes early allocation, repeat initialization, exhaustion,
  and 1,000 release/reallocate cycles.
- Four launcher tests pass: host/join setup, malformed address/port rejection,
  inherited test-setting removal and game exit-code propagation.
- Build guards pass: hosted-state coverage, face cycles, and global bounds.
- Clean-package test passes: cartridge extraction, required-asset checks,
  300 title-screen frames, rendered output, and missing-asset refusal with
  exit code 2 plus an actionable startup error.

`tools/port_refcheck.py` cannot finish on the unmodified release CMake syntax:
its parser rejects `cmake_parse_arguments`. The 234 manifest references it can
check pass. No source renames or moves were introduced here. Retail ROM-byte
and independent source-reconstruction gates were not run, since this changes
the PC port rather than reconstructed retail sources.

## Limits

Tests are bounded and were run on one Windows machine with a European dump.
They do not establish stability across every level, hardware configuration,
long session, packet-loss pattern, or more than two network peers. Real-LAN
latency, internet play and all multiplayer door combinations remain unverified.
Only ROM-free binaries, scripts and metadata belong in downloadable archives.
Existing legacy binaries have not been cleared for redistribution.

## Reproduce

After preparing the upstream build dependencies and your local cartridge data:

```powershell
tools/portable_kit/package_kit.ps1
port/build-local.cmd
python port/tools/coop_stability_proof.py build/port-kit/walk_window.exe
python port/tools/party_proof.py --exe build/port-kit/walk_window.exe --out build/party-proof
python port/tools/party_proof.py --exe build/port-kit/walk_window.exe --out build/party-direct-proof --direct --level 1
python port/tools/test_resource_packs.py build/port-kit/resource_pack_probe.exe
python port/tools/test_coop_launcher.py tools/portable_kit
python port/tools/kit_smoke.py build/kit/SM64DS-Coop-0.5.4-preview YOUR_DUMP.nds --keep --live 0
```
