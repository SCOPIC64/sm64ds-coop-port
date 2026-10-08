# Lua resource packs

Resource packs use the sandboxed declarative Lua API. They cannot open files,
use the network, start processes, inspect game memory, or run general gameplay
scripts. Models and animations must use the port's native BMD/BCA renderer;
texture replacements are PNG files consumed by the existing HD-texture path.

## API v2

Each pack lives in `mods/resource-packs/<pack-id>/pack.lua`. The directory name
is its stable ID. IDs and character-local keys contain only letters, digits,
`-`, and `_`.

```lua
sm64ds.pack {
  id = "example-pack",
  name = "Example Pack",
  author = "Example Author",
  version = "1.0.0",
  license = "CC-BY-4.0",
  provenance = "https://example.invalid/revision/abc123"
}

sm64ds.character {
  key = "hero",                 -- stable network/save key is example-pack:hero
  name = "Example Hero",
  base = 2,                      -- retail gameplay profile; 0..3 only
  body = "models/body.bmd",
  head_cap = "models/head_cap.bmd",
  head_no_cap = "models/head_no_cap.bmd",
  hitbox = { radius = 48, height = 116, hurt_radius = 48, hurt_height = 116 },
  animations = { idle = "anims/idle.bca", run = "anims/run.bca" },
  preview = { animation = "idle", icon = "icon.png", yaw = 15, distance = 320 }
}
```

Numeric `id` remains accepted for v1 packs, but stable keys are authoritative.
The loader assigns runtime IDs 4..255 in enabled pack order and permanently
reserves 0..3 for Mario, Luigi, Wario, and Yoshi. Duplicate keys, texture
hashes, invalid paths, missing assets, oversized scripts, and runaway scripts
reject only the offending pack.

The in-game F5 menu's **mods** rows browse packs, save enable state, and reload.
Reload requests made in a level are queued until a safe menu. State is written
atomically to `resource-packs.state` beside `settings.json` and the executable.

This is an asset-oriented manifest format, not a general gameplay scripting API.

Lua is deliberately declarative: the sandbox exposes no filesystem, network,
process, debug, game-memory, or native-code APIs. Each manifest is capped at
1 MiB, 1,000,000 VM instructions, and 16 MiB of Lua-managed memory. Models use
native `.bmd`, animations use native `.bca`, and texture replacements use PNG
through the existing HD-texture renderer. Paths cannot leave their pack.

The default root is `mods/resource-packs` beside the game process. Set
`SM64DS_RESOURCE_PACKS` to use another exact directory. Prefix a pack directory
with `off_` to disable it without deleting it. Third-party assets are not bundled
merely because a manifest references them.
