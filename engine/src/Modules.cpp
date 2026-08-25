#include "Levi/Modules.h"
#include "Levi/Camera2D.h"
#include "Levi/Physics2D.h"

#include <SDL3/SDL.h>

#include <cstdint>

namespace Levi {

    namespace {
        struct CameraFrameCache {
            std::int64_t frame = -1;
            CameraView2D view;
        };
    } // namespace

    TransformModule::TransformModule(flecs::world& world) {
        world.module<TransformModule>();
        world.component<Position2D>();
        world.component<Scale2D>();
        world.component<Rotation2D>();
        world.component<AABBCollider2D>();
        world.component<CircleCollider2D>();
        world.component<RigidBody2D>();
    }

    CameraModule::CameraModule(flecs::world& world) {
        world.module<CameraModule>();
        world.import<TransformModule>();
        world.component<Camera2D>();
        world.set<CameraFrameCache>({});

        world.system<Camera2D>("UpdateCameraShake")
            .kind(flecs::PreUpdate)
            .each([](flecs::entity entity, Camera2D& camera) {
                Camera2DSystem::advanceShake(camera, entity.id(), entity.world().delta_time());
            });
    }

    PhysicsModule::PhysicsModule(flecs::world& world) {
        world.module<PhysicsModule>();
        world.import<TransformModule>();
        Physics2D::initialize(world);

        world.system("StepPhysics2D").kind(flecs::PreUpdate).run([](flecs::iter& iterator) {
            while (iterator.next()) {
                auto world = iterator.world();
                Physics2D::step(world, iterator.delta_time());
            }
        });
    }

    RenderModule::RenderModule(flecs::world& world) {
        world.module<RenderModule>();
        world.import<TransformModule>();
        world.import<CameraModule>();
        world.component<Sprite2D>();
        world.component<RendererRef>();
        world.component<AssetManagerRef>();

        world.system<const Position2D, const Sprite2D>("RenderSprites")
            .with<Scale2D>()
            .optional()
            .with<Rotation2D>()
            .optional()
            .kind(flecs::OnUpdate)
            .each([](flecs::entity entity, const Position2D& position, const Sprite2D& sprite) {
                auto world = entity.world();
                const auto renderer = world.get<RendererRef>();
                const auto assets = world.get<AssetManagerRef>();
                if (!renderer || !renderer->ptr || !assets || !assets->ptr) return;

                CameraView2D cameraView;
                auto* cameraCache = world.get_mut<CameraFrameCache>();
                const std::int64_t frame = world.get_info()->frame_count_total;
                if (cameraCache && cameraCache->frame != frame) {
                    cameraCache->frame = frame;
                    cameraCache->view = {};
                    auto cameraEntity = Camera2DSystem::findActive(world);
                    if (cameraEntity) {
                        const auto* cameraPosition = cameraEntity.get<Position2D>();
                        const auto* camera = cameraEntity.get<Camera2D>();
                        if (cameraPosition && camera) {
                            cameraCache->view = Camera2DSystem::makeView(*cameraPosition, *camera);
                        }
                    }
                }
                if (cameraCache) cameraView = cameraCache->view;

                int viewportWidth = 0;
                int viewportHeight = 0;
                if (!SDL_GetCurrentRenderOutputSize(renderer->ptr, &viewportWidth, &viewportHeight)) return;
                const Vector2 screenPosition = Camera2DSystem::worldToScreen(
                    position, cameraView, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight));
                const float cameraZoom = cameraView.active ? cameraView.zoom : 1.0f;

                SDL_Texture* texture = assets->ptr->loadTexture(sprite.texturePath);
                float width = sprite.size.x * cameraZoom;
                float height = sprite.size.y * cameraZoom;
                if (const auto* scale = entity.get<Scale2D>()) {
                    width *= scale->x;
                    height *= scale->y;
                }

                float angle = 0.0f;
                SDL_FPoint pivot = {width / 2.0f, height / 2.0f};
                if (const auto* rotation = entity.get<Rotation2D>()) {
                    angle = rotation->angle;
                    if (rotation->pivotType == PivotType::Percent) {
                        pivot = {rotation->pivot.x * width, rotation->pivot.y * height};
                    } else {
                        pivot = {rotation->pivot.x, rotation->pivot.y};
                    }
                }
                if (cameraView.active) angle -= cameraView.rotation;

                SDL_FRect destination = {screenPosition.x - pivot.x, screenPosition.y - pivot.y, width, height};
                if (texture) {
                    SDL_RenderTextureRotated(renderer->ptr, texture, nullptr, &destination, static_cast<double>(angle),
                                             &pivot, SDL_FLIP_NONE);
                } else {
                    SDL_SetRenderDrawColor(renderer->ptr, 255, 255, 0, 255);
                    SDL_RenderFillRect(renderer->ptr, &destination);
                }
            });
    }

} // namespace Levi
