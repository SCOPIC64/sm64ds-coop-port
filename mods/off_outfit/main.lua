-- Outfit mod for SM64 DS COOP.
-- Tints the body outfit: 0 Default, 1 Red, 2 Orange, 3 Yellow, 4 Green,
-- 5 Blue, 6 Purple, 7 Shadow. The head keeps its own colors.
-- The F5 menu "outfit color" row and MODS adjust the same value live.
if sm64ds and sm64ds.set_outfit then
    sm64ds.set_outfit(5)
    sm64ds.log("Outfit loaded: Blue (F5 to change)")
end
