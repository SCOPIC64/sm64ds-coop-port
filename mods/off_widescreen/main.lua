-- Widescreen mod for SM64 DS COOP (setting, not gameplay).
-- DISABLED by default: this folder starts with "off_", so the loader
-- skips it. Rename it to "widescreen" to enable.
-- When enabled it opens a 1280x720 window and stretches the 4:3 frame
-- into it. Takes effect at boot. The MODS front page and the F5
-- "widescreen" row toggle the same persisted setting (needs a restart).
if sm64ds and sm64ds.set_widescreen then
    sm64ds.set_widescreen(true)
    sm64ds.log("Widescreen loaded: 1280x720 window (restart to apply)")
end
