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

1. Copy your .nds file into this folder, so it sits next to play.bat.

2. Double-click play.bat.

   The first time you do this it unpacks the game data out of your dump. That
   takes about a minute and creates two folders here, "extracted" and
   "build". It only happens once; after that play.bat starts the game
   straight away.

3. That is all.


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

Keyboard and mouse:

    W A S D or arrows   walk
    Space               jump
    Shift               run
    Ctrl                crouch
    X                   punch
    Q and E             swing the camera around
    R and F             tilt the camera
    C                   put the camera back behind Mario
    Right mouse drag    look around
    Mouse wheel         zoom
    Esc                 quit

Extra keys:

    F1   change camera: chase rig, free camera, DS-exact stepped rotate
    F3   stats overlay (frame rate, where Mario is, what state he is in)
    F5   debug menu: warp to any entrance in the level. Arrows or the d-pad
         move, Enter or A picks. The game pauses while it is open.
    Tab  show or hide the DS bottom screen under the main view


WHAT YOU CAN DO IN IT
---------------------

Mario spawns on the castle grounds. You can walk, run, jump, punch, crouch
and swim around them, with sound, and the DS bottom screen is drawn under the
main view.

This is a port in progress rather than a finished game. There is no star
select, no course entry and no saving yet.


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
    walk_window.exe      the game
    extract_assets.ps1   unpacks your dump; you can also run it on its own
    README.txt           this file

Once you have run it, two more folders appear, both made from your own dump:

    extracted\           the game's files, exactly as your cartridge holds them
    build\assets\        two small index files that name them

Nothing here connects to the internet, installs anything, or changes any
setting on your PC. It reads your dump, writes those two folders next to
itself, and runs.
