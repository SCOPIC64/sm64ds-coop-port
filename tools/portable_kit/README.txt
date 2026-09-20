Super Mario 64 DS - PC port
===========================

This is a work-in-progress PC build of Super Mario 64 DS, made from a
decompilation of the game. It is the real game code running natively on your
PC: the real physics, the real camera, the real Mario.

It contains no game data at all. No models, no textures, no music, no levels.
All of that comes out of a Super Mario 64 DS cartridge that you own.


WHAT YOU NEED
-------------

1. A Windows PC (Windows 10 or 11). Nothing to install.

2. Your own copy of Super Mario 64 DS for the Nintendo DS, and a dump of that
   cartridge saved as a .nds file. Dumping a cartridge you own is done with a
   DS and a dumping tool; how to do that is outside this kit. A copy
   downloaded from the internet is not the same thing and is not supported
   here.

   North American and European cartridges both work.


HOW TO PLAY
-----------

The game opens on a CoopDX-style menu: HOST enters the castle grounds
while hosting a lobby, JOIN opens the lobby page (host locally, or join
a friend by typing their address into the blank Host IP box), OPTIONS
holds controls, name and mods, QUIT leaves. Everything is remembered
between sessions.

TIP: the grounds are big and quiet. Pick a character and a color in MODS,
hop in the moat (mash jump to swim), walk up to a sign, face it and punch
to talk to it (its text auto-advances for now). Castle doors are locked:
their interiors are not loaded in this build yet.

1. Copy your .nds file into this folder, so it sits next to walk_window.exe.

2. Double-click play.bat.

   The first time you do this it unpacks the game data out of your dump. That
   takes about a minute and creates two folders here, "extracted" and
   "build". It only happens once; after that play.bat starts the game
   straight away.

3. That is all.

The executable checks for exactly one .nds file beside walk_window.exe every
time it starts. The ROM is never bundled by the project, and the game will not
boot without it even after the extracted asset cache has been created.
Generated files are stored in %LOCALAPPDATA%\SM64DS rather than this folder.


LUA MODS
--------

Mods live beside the executable in a folder with this shape:

    mods\my_mod\main.lua

Each main.lua is loaded once at startup. Lua support is enabled when the build
finds Lua 5.4 development files; builds without that optional dependency keep
running and report that Lua mods were skipped. The current scripting surface
is intentionally small while the native game API is being exposed.

The included analog_controls mod starts active in the native build. Open the
F5 debug menu and choose "mod: analog input" to toggle analog movement. The
left stick then controls movement direction and speed with a deadzone.

Mods are real here, not settings with costumes on: each mod is a folder
under mods\, and deleting (or off_-prefixing) the folder uninstalls it.
Lua 5.4 is built into the game, so every main.lua actually executes at
boot. ENTER on a MODS row (or F5 mod row) enables/disables that mod by
renaming its folder; arrows adjust its value live. Dim rows are off.

    analog_controls    analog left-stick movement (disable for DS digital)
    off_character_select
                       character screen in MODS (ENTER opens it) with all
                       four characters; F5 switches live mid-run. All four
                       walk, jump and swim with their own speeds; dialogs
                       show no text yet and auto-advance.
    off_outfit         blue body tint (0 Default, 1 Red, 2 Orange, 3 Yellow,
                       4 Green, 5 Blue, 6 Purple, 7 Shadow); the head keeps
                       its colors. F5 "outfit color" changes it live.
    off_speed_boost    150% movement speed (25-300%)
    off_high_jump      150% jump height, no compounding
    off_small_arena    fences you to a small pit around the spawn instead
                       of the full grounds
    camera_preset      analog chase camera; F1 still cycles chase/freecam/
                       DS-exact, F5 "camera dist" picks the rig distance
    off_bottom_compact tiny bottom screen when enabled (default is already
                       a small 1/3 corner panel; TAB hides it)
    off_widescreen     1280x720 window with a TRUE 16:9 projection (hor+:
                       wider view, no stretch) when enabled. Boot applies it.

Rename any mod folder with an "off_" prefix to disable it. See
LUA_MODDING.md for the Lua API (needs a Lua 5.4-enabled build; this kit's
exe runs the native settings either way and skips Lua scripts).


CONTROLS
--------

Xbox controller:

    Left stick          walk
    A                   jump
    X                   run
    B                   punch
    Right trigger       crouch
    Right stick         swing the camera around
    Bumpers             zoom the camera in and out
    Right stick click   same as F1 below

Keyboard and mouse (jump/run/crouch/punch rebindable in OPTIONS):

    W A S D or arrows   walk
    Space               jump (mash it to swim)
    Shift               run
    Ctrl                crouch
    X                   punch (punch a sign while facing it to talk)
    T                   chat (type, Enter sends, Esc closes)
    Q and E             swing the camera around
    R and F             tilt the camera
    C                   put the camera back behind your character
    Right mouse drag    look around
    Mouse wheel         zoom
    Esc                 quit (closes chat/menus first)

Extra keys:

    F1   change camera: chase rig, free camera, DS-exact stepped rotate
    F3   stats overlay (frame rate, where your character is, what state
         it is in, unhosted-state counter)
    F5   debug menu: warp to any entrance, character select, outfit color,
         speed/jump, bottom-screen size, widescreen (next boot), small
         arena, camera distance, fps unlock. Arrows move, ENTER flips the
         mod on that row, and the game pauses while it is open.
    Tab  show or hide the DS bottom screen under the main view


WHAT YOU CAN DO IN IT
---------------------

You spawn on the castle grounds as the selected character (Mario, Luigi,
Wario or Yoshi) in your name and outfit. You can walk, run, jump, punch,
crouch, swim and talk around them, with sound, and the DS bottom screen is
drawn as a small corner panel over the main view. Chat with T; lobbies
carry presence plus chat (host on port 21330, join by IP address -- the
box ships blank, nothing is hardcoded). Peers do not share the world
yet: no remote players, just names and chat.

This is a port in progress rather than a finished game. Castle doors stay
locked (their interiors are not loaded), sign text auto-advances (the DS
text engine is not hosted), and there is no star select, no course entry
and no saving yet.


IF SOMETHING GOES WRONG
-----------------------

"No .nds file in this folder"
    Your dump is not here yet. It has to sit right next to play.bat and its
    name has to end in .nds.

"That is not a Super Mario 64 DS dump"
    The .nds file here is a different game, or the dump is damaged.

"This dump is truncated" or "incomplete"
    The dump did not finish. Dump the cartridge again.

"...but not a revision this build knows"
    The cartridge is a Super Mario 64 DS release this build has not been
    matched against. Nothing to be done from here.

"...cannot be loaded because running scripts is disabled..."
    play.bat normally avoids this. If you do see it, right-click
    extract_assets.ps1, choose Properties, tick Unblock at the bottom, click
    OK, and run play.bat again.

The window opens and closes immediately
    Open a command prompt in this folder and run play.bat from there so the
    message stays on screen.


WHAT IS IN THIS FOLDER
----------------------

    play.bat             starts the game (and unpacks the data the first time)
    sm64ds coop.exe      the game
    logo.bmp             the menu wordmark (delete it for a text menu)
    extract_assets.ps1   unpacks your dump; you can also run it on its own
    README.txt           this file

Once you have run it, two more folders appear, both made from your own dump:

The optional high-resolution executable and extraction script are under
tools\. Generated extracted\ and build\assets\ data is under:

    %LOCALAPPDATA%\SM64DS

Nothing here connects to the internet, installs anything, or changes any
setting on your PC. It reads your dump, writes those two folders next to
itself, and runs.
