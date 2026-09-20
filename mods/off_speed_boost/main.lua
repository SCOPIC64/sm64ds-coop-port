-- Speed Boost mod for SM64 DS COOP.
-- 150 = 1.5x stick magnitude on analog, dash tier on D-pad.
-- Valid range 25..300. Delete or rename this folder to go back to 100%.
-- The F5 menu "mod: speed" row adjusts the same value live.
if sm64ds and sm64ds.set_speed_pct then
    sm64ds.set_speed_pct(150)
    sm64ds.log("Speed Boost loaded: 150% (F5 to change)")
end
