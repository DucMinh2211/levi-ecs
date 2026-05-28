-- Define custom systems for the project

print("[Lua] Registering Systems...")

-- SpinSystem: Rotates entities that have Rotation2D and SpinData
ECS.registerSystem("SpinSystem", {"Rotation2D", "SpinData"}, function(e)
    local active = ECS.getComponentValue(e, "SpinData", "active")
    if not active then return end

    local speed = ECS.getComponentValue(e, "SpinData", "speed")
    local rot = ECS.getRotation(e)
    
    if rot then
        ECS.setRotation(e, rot.angle + speed)
    end
end)

-- BounceSystem: Moves entities up and down
local time = 0
ECS.registerSystem("BounceSystem", {"Position2D"}, function(e)
    -- This is a global time, would be better to have a Time component
    -- but for demo purposes, we can use a local variable in the script scope
    time = time + 0.01 
    
    local pos = ECS.getPosition(e)
    if pos then
        -- Only bounce if it's an "Enemy" (just a logic test)
        -- We could define a "Bouncer" component instead.
        ECS.setPosition(e, pos.x, pos.y + math.sin(time) * 0.5)
    end
end)