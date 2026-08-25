-- Box2D runtime demo.
-- Press Play to spawn the bodies. They are owned by this script environment,
-- so Stop removes them and restores the edit-time scene automatically.

local ball = nil
local ground = nil

local function createBody(name, x, y, width, height, bodyType)
    local entity = ECS.createEntity(name)
    ECS.addPosition(entity, x, y)
    ECS.addRotation(entity, 0)
    ECS.addSprite(entity, "assets/player.jpg", width, height)
    ECS.addAABBCollider(entity, width, height)
    ECS.addRigidBody(entity, bodyType)
    return entity
end

function onInit()
    Physics.setGravity(0, 980)

    -- Original screen-space rendering uses 960,540 as the viewport center.
    -- If the scene has an active camera, place the demo around that camera
    -- instead so the same script stays visible in both rendering modes.
    local centerX, centerY = 960, 540
    local activeCamera = Camera.getActive()
    if activeCamera then
        local cameraPosition = ECS.getPosition(activeCamera)
        if cameraPosition then
            centerX, centerY = cameraPosition.x, cameraPosition.y
        end
    end

    ground = createBody(
        "PhysicsDemo_Ground",
        centerX, centerY + 360,
        900, 48,
        RigidBodyType2D.Static
    )

    createBody(
        "PhysicsDemo_Box",
        centerX + 80, centerY - 260,
        72, 72,
        RigidBodyType2D.Dynamic
    )

    ball = ECS.createEntity("PhysicsDemo_Ball")
    ECS.addPosition(ball, centerX - 80, centerY - 360)
    ECS.addRotation(ball, 0)
    ECS.addSprite(ball, "assets/player.jpg", 64, 64)
    ECS.addCircleCollider(ball, 32)
    ECS.addRigidBody(ball, RigidBodyType2D.Dynamic)

    Physics.setLinearVelocity(ball, 140, 0)
    print("[Physics Demo] Spawned. Press Space to jump.")
end

function onUpdate(_deltaTime)
    if ball and Input.isKeyPressed("SPACE") then
        Physics.applyImpulse(ball, 0, -1.8)
    end

    if ball and ground and Physics.beganContact(ball, ground) then
        Camera.shake(0.35, 0.15)
    end
end
