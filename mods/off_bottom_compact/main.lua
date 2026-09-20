-- Compact Bottom Screen mod for SM64 DS COOP.
-- The DS bottom screen renders as a small corner panel so the main view
-- stays big: 1 = full DS pixels, 2 = half, 3 = third (the default),
-- 4 = tiny. TAB still hides it entirely. F5 "bottom screen" cycles live.
if sm64ds and sm64ds.set_sub_scale then
    sm64ds.set_sub_scale(4)
    sm64ds.log("Compact Bottom Screen loaded: 1/4 size (F5 to change)")
end
