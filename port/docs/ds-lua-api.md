# DS gameplay mods (API 1 preview)

Put a script at `mods/<your-mod>/main.lua`, then select **Mods** from the title
or Start menu and switch it on. Use **Refresh Mods** after editing a script.
New mods start disabled. `mods/enabled.txt` stores the selection. The executable
creates `mods/sm64-movement/main.lua` on first launch and never overwrites edits.
SM64 movement is implemented entirely in that Lua file; switch it off for DS movement.

Single `.lua` files can also go directly in `mods`. For a multi-file mod, put
`main.lua` and your other `.lua` files in one folder. `main.lua` runs first,
then the other files in alphabetical order, sharing the same sandbox and globals.
Use prefixes such as `10-helpers.lua` and `20-hooks.lua` for a chosen order.
Nested folders are not executed. A mod can contain up to 32 scripts totalling
1 MiB; its memory and instruction limits apply across all files. An error in
any file disables the whole mod. ZIP downloads must be extracted into `mods`.

This is a DS API. CoopDX scripts that use its Mario structs or hooks need adapting.
Gameplay hooks currently run in solo adventure only. Network gameplay continues
with native DS movement because the rollback snapshot does not capture Lua state.
Changing mods during a network session is refused. Resource texture packs remain
available independently of gameplay hooks.

## Example

```lua
-- name: Gentle Gravity
-- description: A smaller downward acceleration for solo Mario.
sm64ds.hook("player_update", function(p)
    if p.character == 0 and p.airborne and not p.cutscene then
        p.gravity = -2
    end
end)
```

`sm64ds.hook(event, function)` registers callbacks during script loading. Events:

| Event | When it runs |
| --- | --- |
| `before_player_update` | Before the native Player behavior |
| `player_update` | After native Player behavior |
| `walk` | At the walking speed/turn update |
| `state` | After a native state function; `action` is its DS address |

Player tables are copies, not pointers. Distances and speeds use game units rather
than DS 20.12 fixed point. Yaw uses signed 16-bit angles (65536 for a full turn).

| Writable field | Accepted range |
| --- | --- |
| `horizontal_speed`, `vertical_speed`, `terminal_velocity` | -512 to 512 |
| `gravity` | -64 to 64 |
| `previous_yaw` | -32768 to 32767 |

Read-only fields: `character` (Mario 0, Luigi 1, Wario 2, Yoshi 3), `action`,
`jump_stage`, `facing_yaw`, `desired_yaw`, `launch_speed` (speed before a state
function), `stick` (0..1 in walk), `floor_normal`, `sink_depth`, `airborne`,
`mega`, `wings`, `no_control`, `multiplayer` (the DS VS flag), `cutscene`,
and `jump_flag`. NaN and infinite writes are ignored; finite writes are clamped.
A walk callback can return `true` to consume the native walk update even when
its output equals the previous value. Other hooks continue running in folder order.

`sm64ds.actions` exposes `JUMP_INIT`, `LONG_JUMP_INIT`, `FALL_INIT`, `FALL_MAIN`.
`sm64ds.log(text)` writes a bounded diagnostic. Base, string, table, math and
UTF-8 libraries are available; filesystem, process, package loading, debug,
coroutines, dynamic code loading and random number generators are absent.
Scripts have a 4 MiB memory budget, a 100,000-instruction startup budget and
50,000 instructions per callback. A failing callback discards its writes and
stops that mod while the game and other mods continue. Refresh to retry it.

## Camera controls

Choose **Options > Camera > SM64 Cam**. Q/E or controller bumpers turn by
45 degrees; F/C-Down zooms out. R/C-Up first returns from the far distance,
then enters inspection when standing still. WASD or the left stick looks around;
jump, attack or a C direction exits. V or right-stick click switches Lakitu/Mario.
C resets the camera behind Mario. Mario stays visible. The original N64 constants
are adapted to DS collision geometry; N64-specific level camera volumes are not
part of this port. Reference: https://github.com/n64decomp/sm64/blob/master/src/game/camera.c

Enter, Escape or controller Start opens the adventure pause menu. During native
dialogue these buttons confirm the text instead. Resume with
Resume/Escape/Start. Title and level-clear dialogs retain their native Start input.
Solo pauses simulation; network sessions continue with neutral local controls.

## Texture pack creation

**Mods > Texture Pack Tools > Start Texture Capture**, then resume/play the
levels you want to edit. Bound textures are exported to `texture-work/capture`
as content-hash PNG files. Return to **Create Pack From Capture** to create a
fresh `mods/resource-packs/my-texture-pack-NNN` folder. Existing packs stay intact.
Edit or upscale its `assets/*.png` without changing filenames; restart to load.
The generated `pack.lua` uses the existing DS resource-pack loader. Rename a pack
folder with an `off_` prefix to disable it. **Open Texture Folders** opens both
working locations. Captures contain local game artwork: distribute your own
replacement images, not unedited captures or cartridge data.

This release is a tested Windows preview. Portable menu/Lua/texture tests run on
Linux and macOS; Android/iOS component compilation is not a playable mobile port.

## Speed & Coordinates HUD

The executable also creates `mods/speed-coordinates/main.lua`. Enable **Speed &
Coordinates** in the title screen's Mods menu before starting your session.
It shows your own player in solo and online adventure. Speeds are game units
per simulation tick (30 ticks/sec); coordinates show 12 decimal places, retaining
every bit of the game's 20.12 position. The display sits below the top-right HUD.

The `hud` event runs before presentation with a copied local-player snapshot.
Call `sm64ds.hud_text(text)` inside its callback to add up to 12 printable ASCII
lines of 100 characters each. HUD callbacks cannot change gameplay. Read-only
`x`, `y`, `z` are world coordinates; `x_fixed`, `y_fixed`, `z_fixed` are the
exact integer 20.12 values. These fields are populated for HUD events.
