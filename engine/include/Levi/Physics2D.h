#pragma once

#include "Components.h"
#include <flecs.h>

namespace Levi {

    class Physics2D {
    public:
        static bool overlaps(flecs::entity first, flecs::entity second);
        static bool overlapsAABB(
            float ax, float ay, float aw, float ah,
            float bx, float by, float bw, float bh);
        static bool overlapsCircle(
            float ax, float ay, float ar,
            float bx, float by, float br);
    };

}
