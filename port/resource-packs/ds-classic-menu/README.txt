DS Classic Menu - optional UI texture pack

Uses the exact frames and letter shapes from the supplied 141636.png sheet.
Labels are removed from the frame PNGs and drawn live. Missing letters/digits
are completed in the same outlined pixel style; lowercase uses uppercase shapes.
S and 5 have different shapes. The stock font and menu remain available.

Requires a game build with sm64ds.menu_theme support (preview.5 or later).
The preview.4 crash hotfix by itself does not have this new API.

INSTALL
1. Quit the game.
2. Put the ds-classic-menu folder inside your game folder's mods/resource-packs.
   The path should end in mods/resource-packs/ds-classic-menu/pack.lua.
3. Start the game. The pack applies to native menus, pause menus and controls.
   Gameplay, character textures, saves and online protocol are unchanged.

DISABLE / RETURN TO STOCK
Preview.6 or later: open Mods > Texture Pack Tools > Enable / Disable Packs.
Select ds-classic-menu and switch it OFF, then quit and restart the game.
Use the same menu to switch it back ON. This leaves all pack files intact.

For older builds:
1. Quit the game.
2. Rename ds-classic-menu to off_ds-classic-menu, or move it outside resource-packs.
3. Restart. To enable again, restore the original folder name and restart.
If you disabled it from the new menu, manually remove its .disabled marker too.
Only one menu theme should be enabled at once; conflicting packs are rejected.

EDIT
panel.png, button.png and selected.png are nine-slice frames with a four-pixel
source border. Corners stay intact while interiors stretch to the live menu.
font.png is a 16-column, six-row grid of 18x18 cells for ASCII 32..126.
Transparent pixels stay transparent. Black/white outlines are copied as-is;
pure magenta pixels are replaced with the live label color. Keep 288x108 size.
The loader caches decoded images once; restart after editing. Invalid assets
fall back to the stock menu with a diagnostic rather than preventing startup.

provenance.json records exact crop locations and source SHA256. This pack does
not contain the full source sheet, its title logos, or copyright artwork.
