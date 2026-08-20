#include "Levi/Physics2D.h"

#include <cmath>

namespace Levi {

    bool Physics2D::overlapsAABB(
        float ax, float ay, float aw, float ah,
        float bx, float by, float bw, float bh) {
        return std::abs(ax - bx) * 2.0f < (aw + bw)
            && std::abs(ay - by) * 2.0f < (ah + bh);
    }

    bool Physics2D::overlapsCircle(
        float ax, float ay, float ar,
        float bx, float by, float br) {
        const float dx = ax - bx;
        const float dy = ay - by;
        const float radius = ar + br;
        return dx * dx + dy * dy < radius * radius;
    }

    bool Physics2D::overlaps(flecs::entity first, flecs::entity second) {
        if (!first.is_alive() || !second.is_alive()) return false;
        const auto* firstPosition = first.get<Position2D>();
        const auto* secondPosition = second.get<Position2D>();
        if (!firstPosition || !secondPosition) return false;

        if (const auto* firstCircle = first.get<CircleCollider2D>()) {
            if (const auto* secondCircle = second.get<CircleCollider2D>()) {
                return overlapsCircle(
                    firstPosition->x + firstCircle->offset.x,
                    firstPosition->y + firstCircle->offset.y,
                    firstCircle->radius,
                    secondPosition->x + secondCircle->offset.x,
                    secondPosition->y + secondCircle->offset.y,
                    secondCircle->radius);
            }
        }

        const auto* firstBox = first.get<AABBCollider2D>();
        const auto* secondBox = second.get<AABBCollider2D>();
        if (!firstBox || !secondBox) return false;
        return overlapsAABB(
            firstPosition->x + firstBox->offset.x,
            firstPosition->y + firstBox->offset.y,
            firstBox->size.x, firstBox->size.y,
            secondPosition->x + secondBox->offset.x,
            secondPosition->y + secondBox->offset.y,
            secondBox->size.x, secondBox->size.y);
    }

}
