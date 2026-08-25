#include "Levi/Physics2D.h"

#include <cmath>

int main() {
    if (!Levi::Physics2D::overlapsAABB(0, 0, 10, 10, 4, 0, 10, 10)) return 1;
    if (Levi::Physics2D::overlapsAABB(0, 0, 10, 10, 20, 0, 10, 10)) return 2;
    if (!Levi::Physics2D::overlapsCircle(0, 0, 5, 8, 0, 5)) return 3;

    flecs::world world;
    auto first = world.entity().set<Levi::Position2D>({0, 0}).set<Levi::AABBCollider2D>({{10, 10}, {0, 0}});
    auto second = world.entity().set<Levi::Position2D>({4, 0}).set<Levi::AABBCollider2D>({{10, 10}, {0, 0}});
    if (!Levi::Physics2D::overlaps(first, second)) return 4;
    second.set<Levi::Position2D>({30, 0});
    if (Levi::Physics2D::overlaps(first, second)) return 5;

    Levi::Physics2D::initialize(world);
    Levi::Physics2D::setGravity(world, 0.0f, 980.0f);
    const auto gravity = Levi::Physics2D::getGravity(world);
    if (gravity.x != 0.0f || gravity.y != 980.0f) return 6;

    Levi::RigidBody2D groundBody;
    groundBody.type = Levi::RigidBodyType2D::Static;
    auto ground = world.entity("PhysicsGround")
                      .set<Levi::Position2D>({0.0f, 100.0f})
                      .set<Levi::AABBCollider2D>({{200.0f, 20.0f}, {0.0f, 0.0f}})
                      .set<Levi::RigidBody2D>(groundBody);

    Levi::RigidBody2D dynamicBody;
    dynamicBody.type = Levi::RigidBodyType2D::Dynamic;
    dynamicBody.fixedRotation = true;
    auto ball = world.entity("PhysicsBall")
                    .set<Levi::Position2D>({0.0f, 0.0f})
                    .set<Levi::CircleCollider2D>({10.0f, {0.0f, 0.0f}})
                    .set<Levi::RigidBody2D>(dynamicBody);

    bool beganContact = false;
    for (int i = 0; i < 180; ++i) {
        Levi::Physics2D::step(world, 1.0f / 60.0f);
        beganContact = beganContact || Levi::Physics2D::beganContact(world, ball.id(), ground.id());
    }
    const auto* settled = ball.get<Levi::Position2D>();
    if (!settled || settled->y < 75.0f || settled->y > 82.0f) return 7;
    if (!beganContact || !Levi::Physics2D::isTouching(world, ball.id(), ground.id())) return 8;

    if (!Levi::Physics2D::setLinearVelocity(world, ball.id(), 120.0f, -300.0f)) return 9;
    const auto velocity = Levi::Physics2D::getLinearVelocity(world, ball.id());
    if (std::abs(velocity.x - 120.0f) > 0.01f || std::abs(velocity.y + 300.0f) > 0.01f) return 10;
    if (!Levi::Physics2D::applyForce(world, ball.id(), 1.0f, 0.0f)) return 11;
    if (!Levi::Physics2D::applyImpulse(world, ball.id(), 0.0f, -0.1f)) return 12;

    const auto ballId = ball.id();
    ball.destruct();
    Levi::Physics2D::step(world, 0.0f);
    if (!Levi::Physics2D::endedContact(world, ballId, ground.id())) return 13;

    Levi::Physics2D::reset(world);

    return 0;
}
