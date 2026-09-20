-- High Jump mod for SM64 DS COOP.
-- Scales the rising velocity once on the jump edge, so there is no
-- compounding: 150 here jumps 1.5x as high, 50 would be a low-gravity hop.
-- Valid range 25..300. The F5 menu "mod: jump" row adjusts it live.
if sm64ds and sm64ds.set_jump_pct then
    sm64ds.set_jump_pct(150)
    sm64ds.log("High Jump loaded: 150% (F5 to change)")
end
