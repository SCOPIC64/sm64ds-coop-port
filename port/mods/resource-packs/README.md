# Lua resource packs

This is the supported high-level mod format in 64DS-DX. It is asset-oriented,
not a general gameplay scripting API, and it contains no Zig component.

Each enabled subdirectory contains a `pack.lua`. Prefix the directory with
`off_` to disable it. Lua is deliberately declarative: it registers native
SM64DS resources and metadata, but cannot access the filesystem, network,
process, debug library, game memory, or arbitrary native code.
Each manifest is also capped at 1 MiB, 1,000,000 VM instructions, and 16 MiB
of Lua-managed memory, so a broken pack cannot hang startup or consume memory
without limit.

```lua
sm64ds.character {
  id = 4,
  name = "Example character",
  base = 2, -- 0 Mario, 1 Luigi, 2 Wario, 3 Yoshi
  body = "assets/body.bmd",
  head_cap = "assets/head-cap.bmd",
  head_no_cap = "assets/head-no-cap.bmd",
  hitbox = {
    radius = 50,
    height = 100,
    hurt_radius = 45,
    hurt_height = 95,
  },
  animations = {
    idle = "assets/idle.bca",
    run = "assets/run.bca",
  },
}

sm64ds.texture {
  -- Content hash printed by SM64DS_HD_TEXTURES_DUMP/INDEX.
  target = "0123456789abcdef",
  source = "assets/body.png",
}
```

Models must be native `.bmd`, animations native `.bca`, and replacement
textures PNG. Texture targets are the existing v0.4 content hashes, so Lua
packs travel through the established HD-texture renderer. Paths cannot leave
the pack directory. Character IDs 0 through 3 remain reserved for Mario,
Luigi, Wario, and Yoshi.

Texture declarations are consumed by the renderer today. Character declarations
are validated and registered for the native character-loading bridge; packs must
not assume an ID is selectable until that bridge reports it as available.

The default pack root is `mods/resource-packs` beside the game process. Set
`SM64DS_RESOURCE_PACKS` to use another exact directory. Each enabled immediate
subdirectory needs a `pack.lua`; prefix its directory name with `off_` to disable
it without deleting it.

Third-party assets are not bundled merely because a pack references them. Pack
authors are responsible for having permission to distribute every asset.
