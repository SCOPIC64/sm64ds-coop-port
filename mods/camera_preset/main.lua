-- Camera Preset mod for SM64 DS COOP.
-- Picks the boot camera: 0 = analog chase (default), 1 = sm64 Lakitu,
-- 2 = freecam, 3 = DS-exact stepped rotate. F1 still cycles live, and
-- F5 has a camera-dist preset row (70/85/100/120/150%).
if sm64ds and sm64ds.set_camera then
    sm64ds.set_camera(0)
    sm64ds.log("Camera Preset loaded: analog chase (F1 to cycle)")
end
