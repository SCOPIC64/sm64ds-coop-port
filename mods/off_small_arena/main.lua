-- Small Arena mod for SM64 DS COOP.
-- Confines the player to a 1200-unit circle around the spawn: a tight
-- coop pit instead of the full castle grounds. No collision is touched,
-- it is a soft fence (position pulled back inside, velocity kept).
-- Toggle live with the F5 menu "small arena" row.
if sm64ds and sm64ds.set_arena then
    sm64ds.set_arena(true)
    sm64ds.log("Small Arena loaded: fenced to the spawn circle (F5 to toggle)")
end
