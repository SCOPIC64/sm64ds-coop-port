-- name: Speed & Coordinates
-- description: Local horizontal/vertical speed and exact world coordinates.
-- author: SM64DS Co-op contributors

-- Speeds are game units per simulation tick (30 ticks per second).
-- 12 decimal places preserve every step of the game's 20.12 coordinates.
sm64ds.hook("hud", function(p)
    sm64ds.hud_text(string.format("Speed: %.4f   Vertical: %.4f", p.horizontal_speed, p.vertical_speed))
    sm64ds.hud_text(string.format("X: %.12f", p.x))
    sm64ds.hud_text(string.format("Y: %.12f", p.y))
    sm64ds.hud_text(string.format("Z: %.12f", p.z))
end)
