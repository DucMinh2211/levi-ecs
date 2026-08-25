#pragma once

#include "Components.h"

#include <flecs.h>

namespace Levi {

    struct CameraView2D {
        Vector2 position = {0.0f, 0.0f};
        float zoom = 1.0f;
        float rotation = 0.0f;
        bool active = false;
    };

    class Camera2DSystem {
    public:
        static constexpr float MinZoom = 0.01f;
        static constexpr float MaxZoom = 100.0f;

        static float clampZoom(float zoom);
        static Vector2 worldToScreen(Vector2 worldPosition, const CameraView2D& camera, float viewportWidth,
                                     float viewportHeight);

        static CameraView2D makeView(const Position2D& position, const Camera2D& camera);
        static flecs::entity findActive(flecs::world& world);
        static void setActive(flecs::world& world, flecs::entity_t entityId);
        static bool move(flecs::world& world, float deltaX, float deltaY);
        static bool setPosition(flecs::world& world, float x, float y);
        static bool setZoom(flecs::world& world, float zoom);
        static float getZoom(flecs::world& world);
        static bool shake(flecs::world& world, float intensity, float duration);
        static void advanceShake(Camera2D& camera, flecs::entity_t entityId, float deltaTime);
    };

} // namespace Levi
