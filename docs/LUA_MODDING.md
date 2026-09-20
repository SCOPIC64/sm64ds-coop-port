# SM64 DS COOP Lua Mods

Lua 5.4.7 is built from source into the game (see `port/vendor/lua`), so
mods always execute -- nothing to install.

## Install a mod

Create a folder under `mods` beside `sm64ds coop.exe`:

```text
SM64 DS COOP/
  mods/
    my_mod/
      main.lua
```

The launcher loads every `main.lua` when the game starts. A script error is
reported in the console and stops startup.

To disable a mod without deleting it, rename its folder with an `off_`
prefix (or a leading `.`): `off_my_mod` is skipped at startup. The MODS
menu does the same rename when you press ENTER on a mod row, and dims rows
whose mod is off. Deleting a mod folder uninstalls it for good.

## Included mods

Gameplay mods ship mostly DISABLED (enable one in MODS and it applies from
the next boot; your menu choices always persist). When enabled, each sets a
startup value; F5 and MODS adjust the same values live and persist them to
`%LOCALAPPDATA%\SM64DS\mod_state.cfg`:

- `off_character_select`: boot as Luigi when enabled (0 Mario, 1 Luigi,
  2 Wario, 3 Yoshi -- edit the number). MODS opens a dedicated character
  screen; F5 "character" switches live mid-run through the ROM's own
  `Player::SetRealCharacter`. All four are fully playable (walk/jump/swim,
  per-character speeds and voice groups); Yoshi's entrance text
  auto-advances like every other dialog.
- `off_outfit`: blue body tint when enabled (F5 "outfit color" and MODS
  "Color" change it live; the head keeps its colors).
- `off_speed_boost`: 150% stick magnitude (dash tier on D-pad).
- `off_high_jump`: 150% rise velocity, applied once on the jump edge so it
  never compounds.
- `off_sm64_movement`: N64-feeling movement preset (speed 130%, jump 115%)
  with its own MODS row; pairs with the F1 sm64 camera. Also restores the
  N64 dive: run + punch becomes a flat fast jump that keeps dash speed
  (DS dropped the dive).
- `off_small_arena`: fences the player to a 1200-unit circle around the
  spawn, a tight coop pit instead of the full castle grounds.
- `camera_preset`: always on; boots the analog chase camera. F1 still
  cycles analog / sm64 / freecam / DS-exact; F5 "camera dist" picks
  70/85/100/120/150%.
- `analog_controls`: always on; analog left-stick movement (disable it for
  DS-style digital movement).
- `off_bottom_compact`: bottom screen at 1/4 size when enabled (default
  without it is 1/3, already small so the main view stays big). TAB hides
  it.
- `off_widescreen`: 1280x720 window with a TRUE 16:9 projection (hor+:
  the sides gain picture instead of stretching) when enabled. Takes
  effect at boot.
- `outfit`: blue body tint (F5 "outfit color" and MODS change it live).

Native / template:

- `analog_controls`: analog left-stick movement. Starts enabled; F5
  `mod: analog input` toggles it.
- `starter_template`: a minimal starting point for writing a mod.
- `camera_notes`: an example metadata mod that points to the native F1 camera
  modes.

When several mods set the same key, the last one loaded wins. Mods set
different keys, so the shipped set composes.

## Lua API

```lua
sm64ds.log("message shown in the game console")
local rom = sm64ds.rom_path

sm64ds.set_character(1)    -- 0 Mario, 1 Luigi, 2 Wario, 3 Yoshi
sm64ds.get_character()     -- last requested id, or nil when untouched
sm64ds.set_speed_pct(150)  -- 25..300
sm64ds.set_jump_pct(150)   -- 25..300
sm64ds.set_sub_scale(4)    -- bottom screen divisor 1..4
sm64ds.set_widescreen(true)
sm64ds.set_arena(true)
sm64ds.set_camera(0)       -- 0 analog, 1 sm64, 2 freecam, 3 DS-exact
sm64ds.set_outfit(5)       -- 0..7 outfit tint
sm64ds.mod_enabled("id")   -- is a mod folder enabled? (e.g. "speed_boost")
```

`sm64ds.rom_path` is the path to the required local `.nds` file. Lua scripts
run once at startup; the values they set are applied at boot (character,
widescreen, camera) and every frame (speed, jump, arena, bottom screen).
Per-frame callbacks, actor spawning and memory access are not exposed yet;
those need native game API bindings.

## Controls

- F5: open or close the in-game menu; the game always starts in gameplay
- Up/Down: select a menu row
- Left/Right or Enter: change or activate a row
- FRONT MODS page (game start): same settings before you spawn
- TAB: hide/show the bottom-screen panel
- F1: cycle camera analog -> sm64 -> freecam -> DS-exact
- F3: stats overlay (now also shows character and speed)

The ROM and mods stay beside the executable; generated extracted data stays
in `%LOCALAPPDATA%\SM64DS`.
