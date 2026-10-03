SM64DS Co-op - 0.5.4 preview
==========================

An experimental Windows port based on tangosdev/sm64ds-decomp's
release/game-0.5.4 branch (7366f2568). This uses that release's paired
decompilation and PC support code, not the newer, incompatible main branch.
This is not SM64CoopDX and does not connect to SM64CoopDX servers.

PLAY
----
Put a dump of your own European or North American Super Mario 64 DS
cartridge in PLACE EU ROM HERE, then double-click play.bat. Despite the
folder name, supported North American dumps are accepted too.
The first launch extracts the data locally. No game data is included here.
Windows 10 or 11 is required. The C++ runtime is linked into the executable.

This portable preview does not include SM64DSLauncher.exe or a room-code
service. Network co-op remains experimental; do not assume that every
level, transition, or multiplayer session works.

For two-player co-op on a trusted local network, both computers need the
same preview and their own extracted data. Start host-coop.bat on one, then
join-coop.bat on the other and enter the host's IPv4 address. Both start in
the castle grounds. The host listens on UDP 51765. Allow the game through
Windows Firewall only on your private network if Windows asks. These
shortcuts do not change firewall rules, forward router ports, or provide
public matchmaking. Do not expose this experimental service to the internet.
Co-op saves use separate coop-host.sav and coop-join.sav files.
If Windows blocks a downloaded coop.ps1, use its Properties > Unblock on
your own PC. On a managed PC, ask the administrator instead.

CONTROLS AND DEFAULTS
--------------------
WASD / arrows: move. Space: jump. Ctrl: crouch. X: punch.
Q/E: turn camera. R/F: tilt camera. C: center camera.
Right mouse drag: look. Mouse wheel: zoom. Alt+F4: quit.
Xbox pad: left stick moves, A jumps, B punches, right trigger crouches,
right stick turns the camera. F4 changes character.
F5 opens the single-player debug menu; Tab toggles the lower screen.

The supplied settings.json selects analog movement and camera, widescreen,
60 FPS with smooth motion, player names, and rollback networking. Voice and
automatic mouse capture are off. Repackaging preserves existing settings.
These are convenience defaults, not a claim of SM64CoopDX feature parity.

SAVES AND TROUBLESHOOTING
-----------------------
Keep this preview in its own folder. Do not overwrite an older installation
or copy old save states between versions. Back up any saves before testing.
Keep the extracted and build folders together with the executable.
If startup fails, run play.bat and inspect the error and playlog folder.
An incomplete extraction is regenerated when a required asset is missing.

STATUS
------
This is a development preview, not a stable release or a bug-free build.
Only source, scripts, metadata and ROM-free builds may be published.
Never include your .nds dump, extracted folder, saves, or build/assets in
a release archive.

Project: https://github.com/SCOPIC64/sm64ds-coop-port
Upstream: https://github.com/tangosdev/sm64ds-decomp
