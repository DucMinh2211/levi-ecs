#include "Levi/Camera2D.h"

#include <cmath>

namespace {
    bool near(float first, float second) {
        return std::fabs(first - second) < 0.001f;
    }
}

int main() {
    Levi::CameraView2D view;
    view.active = true;
    view.position = {100.0f, 50.0f};
    view.zoom = 2.0f;

    auto screen = Levi::Camera2DSystem::worldToScreen({110.0f, 55.0f}, view, 200.0f, 100.0f);
    if (!near(screen.x, 120.0f) || !near(screen.y, 60.0f)) return 1;

    view.rotation = 90.0f;
    screen = Levi::Camera2DSystem::worldToScreen({110.0f, 50.0f}, view, 200.0f, 100.0f);
    if (!near(screen.x, 100.0f) || !near(screen.y, 30.0f)) return 2;

    Levi::CameraView2D identity;
    screen = Levi::Camera2DSystem::worldToScreen({13.0f, 17.0f}, identity, 200.0f, 100.0f);
    if (!near(screen.x, 13.0f) || !near(screen.y, 17.0f)) return 3;
    if (!near(Levi::Camera2DSystem::clampZoom(0.0f), Levi::Camera2DSystem::MinZoom)) return 4;

    flecs::world world;
    auto first = world.entity("FirstCamera")
        .set<Levi::Position2D>({10.0f, 20.0f})
        .set<Levi::Camera2D>({});
    auto secondCamera = Levi::Camera2D{};
    secondCamera.active = false;
    auto second = world.entity("SecondCamera")
        .set<Levi::Position2D>({50.0f, 60.0f})
        .set<Levi::Camera2D>(secondCamera);

    if (Levi::Camera2DSystem::findActive(world) != first) return 5;
    Levi::Camera2DSystem::setActive(world, second.id());
    if (Levi::Camera2DSystem::findActive(world) != second) return 6;
    if (!Levi::Camera2DSystem::move(world, 5.0f, -5.0f)) return 7;
    const auto* position = second.get<Levi::Position2D>();
    if (!position || !near(position->x, 55.0f) || !near(position->y, 55.0f)) return 8;
    if (!Levi::Camera2DSystem::setZoom(world, 3.0f)
        || !near(Levi::Camera2DSystem::getZoom(world), 3.0f)) return 9;

    if (!Levi::Camera2DSystem::shake(world, 1.0f, 0.5f)) return 10;
    auto* camera = second.get_mut<Levi::Camera2D>();
    Levi::Camera2DSystem::advanceShake(*camera, second.id(), 0.1f);
    if (camera->shakeRemaining <= 0.0f
        || (near(camera->shakeOffset.x, 0.0f) && near(camera->shakeOffset.y, 0.0f))) return 11;
    Levi::Camera2DSystem::advanceShake(*camera, second.id(), 1.0f);
    if (!near(camera->shakeRemaining, 0.0f)
        || !near(camera->shakeOffset.x, 0.0f)
        || !near(camera->shakeRotation, 0.0f)) return 12;

    return 0;
}
