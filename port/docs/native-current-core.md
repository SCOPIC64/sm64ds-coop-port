# Current renderer in the native core

The native foundation now uses the renderer, picture interfaces, model sampler
and memory diagnostics from the working Windows build 116. Its coroutine
backend remains available on POSIX hosts. This keeps new native work from
replacing recent rendering and memory-protection changes with the older core.

The renderer includes the isolated geometry-preview interface and the bounded
texture/source metadata used by the current character models. The BCA sampler
is shared with model previews. Windows fixed-memory reservations retain the
strict executable-image ownership checks and per-region allocation diagnostics.
The non-Windows allocation hint uses the portable path-buffer bound.

The staged 32-bit Windows core passes actual fixed I/O and DS state-section checks,
nested VBlank and 11,309 HBlank deliveries, stop/restart and singleton reentry.
Its separate frame-context test preserves two interleaved stacks across 1,200
frames. These are component tests, not a native adventure boot. The 32-bit Linux core,
sprite, allocator, Player compilation and SDL integration run in CI; the desktop
workflow now also runs when the core changes.

No game package, cartridge assets, saves or credentials are added. This change
does not transplant the Windows menu/game host into a native application.
Complete native boot, preferences/imports/lobby integration and gameplay/save
tests remain required. The 64-bit macOS and mobile hosts still need the guest/host
pointer and call representation; layout guards remain enabled. SteamOS behavior
and mixed-platform multiplayer must be tested after the Linux adventure runs.
