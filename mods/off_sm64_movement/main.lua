-- SM64 Movement mod for SM64 DS COOP.
-- N64-feeling movement as a preset in one place: a hotter run (130 is about
-- a third over DS base, where N64 Mario's 64 u/f top speed lives) and a
-- slightly higher jump. Valid ranges 25..300; tune the two numbers below.
-- You still have to ask for it: full speed needs the run button (or deep
-- stick tilt past 72%, same as vanilla) -- a light tilt always stays a
-- walk, so the run button keeps meaning something.
-- DIVE: DS dropped N64's dive, so run + punch gets you the ROM's own jump
-- instead, with N64 dive ballistics held while airborne (entry speed kept
-- exactly, flat +20u rise). Standing punches still talk/punch.
-- Honest limits: scalars move top speed and jump height only. The rest of
-- the N64 feel (8 u/f^2 accel, 0x800/frame face turns) lives in ROM
-- behavior code and needs native hooks -- a future mod.
-- Ships OFF: ENTER on its MODS row enables it (restart applies the script).
-- The F5 "mod: speed" / "mod: jump" rows adjust the same values live.
if sm64ds and sm64ds.set_speed_pct then
    sm64ds.set_speed_pct(130)
    sm64ds.log("SM64 Movement loaded: speed 130% (F5 to change)")
end
if sm64ds and sm64ds.set_jump_pct then
    sm64ds.set_jump_pct(115)
    sm64ds.log("SM64 Movement loaded: jump 115% (F5 to change)")
end
-- Last writer wins: speed_boost / high_jump load after this mod and would
-- override its half, so say so instead of failing silently.
if sm64ds and sm64ds.mod_enabled then
    if sm64ds.mod_enabled("speed_boost") then
        sm64ds.log("SM64 Movement note: speed_boost is also on, it wins")
    end
    if sm64ds.mod_enabled("high_jump") then
        sm64ds.log("SM64 Movement note: high_jump is also on, it wins")
    end
end
