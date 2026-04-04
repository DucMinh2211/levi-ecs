#pragma once

#include <flecs.h>

namespace Levi {

    class Inspector {
    public:
        Inspector() = default;
        ~Inspector() = default;

        void render(flecs::entity entity);
    };

}
