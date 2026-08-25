#pragma once

#include "Components.h"
#include <flecs.h>

namespace Levi {

    class Physics2D {
    public:
        static constexpr float DefaultPixelsPerMeter = 100.0f;

        static void initialize(flecs::world& world);
        static void reset(flecs::world& world);
        static void step(flecs::world& world, float deltaTime);

        static void setGravity(flecs::world& world, float x, float y);
        static Vector2 getGravity(flecs::world& world);

        static bool setLinearVelocity(flecs::world& world, flecs::entity_t entityId, float x, float y);
        static Vector2 getLinearVelocity(flecs::world& world, flecs::entity_t entityId);
        static bool applyForce(flecs::world& world, flecs::entity_t entityId, float x, float y);
        static bool applyImpulse(flecs::world& world, flecs::entity_t entityId, float x, float y);

        static bool isTouching(flecs::world& world, flecs::entity_t first, flecs::entity_t second);
        static bool beganContact(flecs::world& world, flecs::entity_t first, flecs::entity_t second);
        static bool endedContact(flecs::world& world, flecs::entity_t first, flecs::entity_t second);

        static bool overlaps(flecs::entity first, flecs::entity second);
        static bool overlapsAABB(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh);
        static bool overlapsCircle(float ax, float ay, float ar, float bx, float by, float br);
    };

} // namespace Levi
