#include "Levi/Modules.h"

int main() {
    flecs::world world;
    world.import<Levi::PhysicsModule>();

    auto body = world.entity()
                    .set<Levi::Position2D>({0.0f, 0.0f})
                    .set<Levi::CircleCollider2D>({10.0f, {0.0f, 0.0f}})
                    .set<Levi::RigidBody2D>({});

    world.progress(1.0f / 60.0f);
    const auto* position = body.get<Levi::Position2D>();
    return position && position->y > 0.0f ? 0 : 1;
}
