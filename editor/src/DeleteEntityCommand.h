#pragma once

#include "UndoRedoManager.h"

#include "Levi/Components.h"
#include "Levi/ScriptComponent.h"

#include <flecs.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Levi {

    class DeleteEntityCommand final : public EditorCommand {
    public:
        explicit DeleteEntityCommand(flecs::entity root);

        void undo() override;
        void redo() override;
        const std::string& name() const override { return name_; }

    private:
        struct EntitySnapshot {
            flecs::entity_t id = 0;
            flecs::entity_t parentId = 0;
            std::string name;
            std::optional<Position2D> position;
            std::optional<Scale2D> scale;
            std::optional<Rotation2D> rotation;
            std::optional<Sprite2D> sprite;
            std::optional<AABBCollider2D> aabb;
            std::optional<CircleCollider2D> circle;
            std::optional<RigidBody2D> rigidBody;
            std::optional<Camera2D> camera;
            std::vector<std::pair<flecs::entity_t, ScriptComponent>> scriptComponents;
        };

        void capture(flecs::entity entity, flecs::entity_t parentId);

        std::string name_ = "Delete Entity";
        flecs::world world_;
        flecs::entity_t rootId_ = 0;
        std::vector<EntitySnapshot> snapshots_;
    };

} // namespace Levi
