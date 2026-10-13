-- name: SM64 Movement
-- description: N64 walking and jump launches adapted to DS Mario.
-- author: SM64DS Co-op contributors

local function active(p)
    return p.character == 0 and not p.multiplayer and not p.cutscene
        and not p.mega and not p.wings and not p.no_control
end

sm64ds.hook("walk", function(p)
    if not active(p) then return end
    local speed = p.horizontal_speed
    local target = 32 * math.max(0, math.min(1, p.stick))
    if p.sink_depth > 10 then target = target * 6.25 / p.sink_depth end
    if p.stick <= 0 then speed = math.max(0, speed - 1)
    elseif speed <= 0 then speed = speed + 1.1
    elseif speed <= target then speed = speed + 1.1 - speed / 43
    elseif p.floor_normal >= 0.95 then speed = speed - 1 end
    p.horizontal_speed = math.min(speed, 48)
    if p.stick > 0 then
        local delta = (p.desired_yaw - p.previous_yaw + 32768) % 65536 - 32768
        p.previous_yaw = (p.previous_yaw + math.max(-2048, math.min(2048, delta)) + 32768) % 65536 - 32768
    end
    return true -- Consume the native walk update even at a stable target speed.
end)

sm64ds.hook("state", function(p)
    if not active(p) then return end
    local a = sm64ds.actions
    if p.action == a.JUMP_INIT and p.airborne then
        p.vertical_speed = p.jump_stage == 2 and 69
            or (p.jump_stage == 1 and 52 or 42) + p.launch_speed / 4
        if not p.jump_flag then p.horizontal_speed = p.launch_speed * 0.8 end
        p.gravity = -4; p.terminal_velocity = -75
    elseif p.action == a.LONG_JUMP_INIT and p.airborne then
        p.vertical_speed = 30
        p.horizontal_speed = math.min(p.launch_speed * 1.5, 48)
        p.gravity = -2; p.terminal_velocity = -75
    elseif p.action == a.FALL_INIT or p.action == a.FALL_MAIN then
        p.gravity = -4; p.terminal_velocity = -75
    end
end)
