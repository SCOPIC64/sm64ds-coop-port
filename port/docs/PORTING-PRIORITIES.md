# Next priority after the seven-player Windows release

Native Linux/SteamOS and macOS game ports are the highest priority. Android
and iOS follow those desktop ports. This ordering was selected by the project
owner on 2026-10-12. New optional Windows features should wait while desktop
portability work is active; release-blocking multiplayer regressions still
need repair.

The existing cross-platform menu, media, input, gameplay-model and native-memory
checks are a foundation. They are not playable game releases. Keep the tested
seven-player Windows binary available while replacing the remaining engine
platform dependencies.

1. Audit the four-byte engine ABI, MSVC forwarding and member-function pointer
   representation before choosing the Linux and macOS compiler/architecture
   strategy. Test pointer/layout and calling-convention assumptions explicitly.
2. Implement native window/render, input/controller, audio, sockets, threading,
   process/file-dialog and profile-storage adapters. Replace Windows DPAPI with
   appropriate native account storage without changing signed server discovery.
3. Boot the full game with the user's own extracted assets on Linux/SteamOS
   and macOS. Validate saves, animation, collisions and return-to-menu behavior.
4. Verify mixed Windows/Linux/macOS rooms, player-owned damage/death/respawn,
   palette imports, chat and independent input. Exercise all seven seats and
   level transitions. Measure game ticks and input buffering on each target.
5. Publish native packages only after full-game and multiplayer verification.
   Do not label component tests, Wine launchers or renamed Windows EXEs as
   native Linux/macOS ports.

SteamOS additionally needs controller-first menus, suspend/resume and Deck
performance testing. macOS needs separate confirmation of the Intel and Apple
Silicon ABI paths, app packaging and native input/audio behavior. Android and
iOS then build on the desktop adapters, adding lifecycle, storage and physical
touchscreen-device testing.
