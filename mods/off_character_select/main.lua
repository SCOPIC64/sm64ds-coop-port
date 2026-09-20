-- Character Select mod for SM64 DS COOP (coop-style).
-- Picks the boot character. Change the number and restart:
--   0 = Mario, 1 = Luigi, 2 = Wario, 3 = Yoshi
-- You can also switch live without restarting: open the F5 debug menu,
-- move to the "character" row, and press Left/Right or Enter.
if sm64ds and sm64ds.set_character then
    sm64ds.set_character(1)
    sm64ds.log("Character Select: booting as Luigi (edit mods/character_select/main.lua to change)")
end
