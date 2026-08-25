#include "Levi/Camera2D.h"

#include <algorithm>
#include <cmath>

namespace Levi {

    float Camera2DSystem::clampZoom(float zoom) {
        return std::clamp(zoom, MinZoom, MaxZoom);
    }

    Vector2 Camera2DSystem::worldToScreen(Vector2 worldPosition, const CameraView2D& camera, float viewportWidth,
                                          float viewportHeight) {
        if (!camera.active) return worldPosition;

        const float radians = -camera.rotation * 0.01745329251994329577f;
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        const float relativeX = worldPosition.x - camera.position.x;
        const float relativeY = worldPosition.y - camera.position.y;
        const float rotatedX = relativeX * cosine - relativeY * sine;
        const float rotatedY = relativeX * sine + relativeY * cosine;
        const float zoom = clampZoom(camera.zoom);
        return {viewportWidth * 0.5f + rotatedX * zoom, viewportHeight * 0.5f + rotatedY * zoom};
    }

    CameraView2D Camera2DSystem::makeView(const Position2D& position, const Camera2D& camera) {
        return {{position.x + camera.shakeOffset.x, position.y + camera.shakeOffset.y},
                clampZoom(camera.zoom),
                camera.shakeRotation,
                camera.active};
    }

    flecs::entity Camera2DSystem::findActive(flecs::world& world) {
        flecs::entity active;
        world.query<const Camera2D, const Position2D>().each(
            [&active](flecs::entity entity, const Camera2D& camera, const Position2D&) {
                if (camera.active && (!active || entity.id() < active.id())) active = entity;
            });
        return active;
    }

    void Camera2DSystem::setActive(flecs::world& world, flecs::entity_t entityId) {
        world.query<Camera2D>().each(
            [entityId](flecs::entity entity, Camera2D& camera) { camera.active = entity.id() == entityId; });
    }

    bool Camera2DSystem::move(flecs::world& world, float deltaX, float deltaY) {
        auto entity = findActive(world);
        if (!entity) return false;
        auto* position = entity.get_mut<Position2D>();
        if (!position) return false;
        position->x += deltaX;
        position->y += deltaY;
        return true;
    }

    bool Camera2DSystem::setPosition(flecs::world& world, float x, float y) {
        auto entity = findActive(world);
        if (!entity) return false;
        entity.set<Position2D>({x, y});
        return true;
    }

    bool Camera2DSystem::setZoom(flecs::world& world, float zoom) {
        auto entity = findActive(world);
        if (!entity) return false;
        auto* camera = entity.get_mut<Camera2D>();
        if (!camera) return false;
        camera->zoom = clampZoom(zoom);
        return true;
    }

    float Camera2DSystem::getZoom(flecs::world& world) {
        auto entity = findActive(world);
        if (const auto* camera = entity ? entity.get<Camera2D>() : nullptr) return clampZoom(camera->zoom);
        return 1.0f;
    }

    bool Camera2DSystem::shake(flecs::world& world, float intensity, float duration) {
        auto entity = findActive(world);
        if (!entity) return false;
        auto* camera = entity.get_mut<Camera2D>();
        if (!camera) return false;
        camera->shakeIntensity = std::max(0.0f, intensity);
        camera->shakeDuration = std::max(0.0f, duration);
        camera->shakeRemaining = camera->shakeDuration;
        camera->shakeClock = 0.0f;
        if (camera->shakeRemaining <= 0.0f || camera->shakeIntensity <= 0.0f) {
            camera->shakeOffset = {};
            camera->shakeRotation = 0.0f;
        }
        return true;
    }

    void Camera2DSystem::advanceShake(Camera2D& camera, flecs::entity_t entityId, float deltaTime) {
        camera.zoom = clampZoom(camera.zoom);
        if (camera.shakeRemaining <= 0.0f || camera.shakeDuration <= 0.0f || camera.shakeIntensity <= 0.0f) {
            camera.shakeRemaining = 0.0f;
            camera.shakeOffset = {};
            camera.shakeRotation = 0.0f;
            return;
        }

        camera.shakeClock += std::max(0.0f, deltaTime);
        camera.shakeRemaining = std::max(0.0f, camera.shakeRemaining - std::max(0.0f, deltaTime));
        if (camera.shakeRemaining <= 0.0f) {
            camera.shakeOffset = {};
            camera.shakeRotation = 0.0f;
            return;
        }

        const float falloff = camera.shakeRemaining / camera.shakeDuration;
        const float amplitude = camera.shakeIntensity * falloff * falloff;
        const float seed = static_cast<float>(flecs::strip_generation(entityId) % 997u) * 0.0137f;
        camera.shakeOffset.x = std::sin(camera.shakeClock * 37.0f + seed) * camera.maxShakeOffset * amplitude;
        camera.shakeOffset.y = std::sin(camera.shakeClock * 53.0f + seed * 1.7f) * camera.maxShakeOffset * amplitude;
        camera.shakeRotation = std::sin(camera.shakeClock * 29.0f + seed * 2.3f) * camera.maxShakeRotation * amplitude;
    }

} // namespace Levi
