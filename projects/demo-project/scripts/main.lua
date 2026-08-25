-- Levi Engine Modular Scripting
-- This file now acts as an entry point to load components and systems

print("=== Levi Engine Modular Scripting Initialized ===")

-- Load modular scripts
-- (Engine scans the folder and loads all .lua files, so these might be redundant 
-- if they are in the same folder, but explicit require is safer for order)

-- require("components")
-- require("systems")

function onInit()
    print("[Lua] Global onInit() called.")
    -- Entities should now be created via Editor, 
    -- but you can still create persistent "Manager" entities here if needed.
    ECS.createEntity("aaaa");
end

function onUpdate(deltaTime)
    -- Global update logic if any
end

function onShutdown()
    print("[Lua] Global onShutdown() called.")
end
