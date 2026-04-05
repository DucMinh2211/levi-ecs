-- Demo Lua Script for Levi Engine
-- This script demonstrates ECS API usage and hot-reloading

print("=== Levi Lua Script Loaded ===")

-- Store entity IDs globally so we can modify them in onUpdate
entities = {}

-- Called once when script loads
function onInit()
    print("[Lua] onInit() - Creating entities...")
    -- local tmp = ECS.createEntity();
    -- ECS.addPosition(tmp, 100, 100)
    -- ECS.addSprite(tmp, "assets/player.jpg", 100, 100)
    
    -- Create a player entity with name
    local player = ECS.createEntity("Player")
    ECS.addPosition(player, 400, 300)
    ECS.addSprite(player, "assets/player.jpg", 100, 100)
    entities.player = player
    
    print("[Lua] Created player entity: " .. player)
    
    -- Create some test entities with names
    for i = 1, 3 do
        local entity = ECS.createEntity("Enemy_" .. i)
        ECS.addPosition(entity, 100 + i * 150, 200)
        ECS.addSprite(entity, "assets/player.jpg", 80, 80)
        table.insert(entities, entity)
    end
    
    print("[Lua] onInit() complete! Created " .. (#entities + 1) .. " entities")
end

-- Called every frame
local time = 0
function onUpdate(deltaTime)
    time = time + deltaTime
    
    -- Move player in a circle (Hot-reload test: change speed here!)
    if entities.player then
        local radius = 100
        local speed = 3.0  -- Try changing this value and saving!
        local centerX = 400
        local centerY = 300
        
        local x = centerX + math.cos(time * speed) * radius
        local y = centerY + math.sin(time * speed) * radius
        
        ECS.setPosition(entities.player, x, y)
    end
    
    -- Move other entities up and down
    for i, entityId in ipairs(entities) do
        local pos = ECS.getPosition(entityId)
        if pos then
            local offset = math.sin(time * 3 + i) * 50
            ECS.setPosition(entityId, pos.x, 200 + offset)
        end
    end
end

-- Called when script is about to be reloaded or engine shuts down
function onShutdown()
    print("[Lua] onShutdown() - Cleaning up...")
    -- Note: You don't need to delete entities here,
    -- they persist across script reloads!
end

print("[Lua] Script functions registered")
