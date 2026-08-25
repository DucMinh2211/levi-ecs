-- Define custom components for the project

print("[Lua] Registering Components...")

-- SpinData: controls how an entity rotates
ECS.defineComponent("SpinData", {
    speed = 1.0,
    active = true
})

-- Health: simple health tracker
ECS.defineComponent("Health", {
    hp = 100,
    maxHp = 100
})

-- Opt-in marker for BounceSystem. Keeping movement systems narrowly queried
-- prevents them from fighting Box2D over physics-owned transforms.
ECS.defineComponent("Bouncer", {
    speed = 1.0
})
