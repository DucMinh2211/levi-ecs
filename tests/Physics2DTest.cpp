#include "Levi/Physics2D.h"

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
    return 0;
}
