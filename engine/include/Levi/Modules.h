#pragma once

#include "AssetManager.h"
#include "Components.h"

#include <flecs.h>

struct SDL_Renderer;

namespace Levi {

    struct RendererRef {
        SDL_Renderer* ptr = nullptr;
    };
    struct AssetManagerRef {
        AssetManager* ptr = nullptr;
    };

    struct TransformModule {
        explicit TransformModule(flecs::world& world);
    };

    struct CameraModule {
        explicit CameraModule(flecs::world& world);
    };

    struct PhysicsModule {
        explicit PhysicsModule(flecs::world& world);
    };

    struct RenderModule {
        explicit RenderModule(flecs::world& world);
    };

} // namespace Levi
