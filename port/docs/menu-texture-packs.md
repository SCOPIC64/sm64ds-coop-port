# Optional native menu texture packs

`sm64ds.menu_theme` extends the existing sandboxed resource-pack registry. It
does not replace the built-in theme or patch cartridge assets. Every enabled
`mods/resource-packs/<name>/pack.lua` may register one menu theme:

```lua
sm64ds.menu_theme {
    panel = "panel.png", button = "button.png", selected = "selected.png",
    font = "font.png", border = 4
}
```

Paths resolve within the declaring pack, including symlink resolution. PNGs
are limited to 8 MiB each and 2048 pixels per dimension before decoding. Border
must be 1..16 and less than half each frame's dimensions. The font is exactly
288x108: 16 columns of 18x18 cells, six rows, ASCII 32..126. Magenta indicates
live text fill; other colors and alpha preserve the glyph's original outlines.
Lowercase characters share uppercase shapes. Glyph advances are calculated
once on load. Images are decoded once, outside gameplay's frame loop.

The renderer draws panel/button frames with nine slices, preserving source
corners. Geometry and hit testing are identical with and without a pack. Menu
text uses the pack's font; chat, diagnostics and player names retain the normal
font. Missing/invalid PNGs select the built-in theme. A malformed Lua pack is
rejected atomically. Only one enabled pack can own the menu; disable competing
themes by moving them out of the resource-pack root or using the `off_` prefix.

`port/tools/extract_ds_menu_pack.py <141636.png> <output>` reproduces the DS
Classic pack from the supplied sheet. It copies borders exactly, removes label
pixels with clean material from the same frame, extracts the available glyphs,
and completes missing glyphs in matching outlined pixel style. It never embeds
the entire sheet or unrelated logos. `provenance.json` records source crops and
hash. `asset-proof.png` is a source-asset inspection image, not a game screenshot
and should not be included in the distributable pack.

Verification includes stock and textured render bounds on landscape/portrait
surfaces, menu-theme registration/conflicts/path confinement, real executable
menu captures with the pack enabled/disabled, and invalid-image fallback.
