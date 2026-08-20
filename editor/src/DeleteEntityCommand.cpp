#include "DeleteEntityCommand.h"

namespace Levi {

    DeleteEntityCommand::DeleteEntityCommand(flecs::entity root)
        : world_(root.world()), rootId_(root.id()) {
        if (!root || !root.is_alive()) return;

        capture(root, root.parent().id());
    }

    void DeleteEntityCommand::capture(flecs::entity entity, flecs::entity_t parentId) {
        EntitySnapshot snapshot;
        snapshot.id = entity.id();
        snapshot.parentId = parentId;
        snapshot.name = entity.name().c_str();

        if (const auto* value = entity.get<Position2D>()) snapshot.position = *value;
        if (const auto* value = entity.get<Scale2D>()) snapshot.scale = *value;
        if (const auto* value = entity.get<Rotation2D>()) snapshot.rotation = *value;
        if (const auto* value = entity.get<Sprite2D>()) snapshot.sprite = *value;
        if (const auto* value = entity.get<AABBCollider2D>()) snapshot.aabb = *value;
        if (const auto* value = entity.get<CircleCollider2D>()) snapshot.circle = *value;
        if (const auto* value = entity.get<Camera2D>()) snapshot.camera = *value;

        const flecs::entity_t scriptComponentId = world_.component<ScriptComponent>().id();
        entity.each(scriptComponentId, flecs::Wildcard, [&snapshot, &entity](flecs::id id) {
            const flecs::entity schema = id.second();
            if (const auto* value = entity.get<ScriptComponent>(schema)) {
                snapshot.scriptComponents.emplace_back(schema.id(), *value);
            }
        });

        snapshots_.push_back(std::move(snapshot));
        entity.children([this](flecs::entity child) {
            capture(child, child.parent().id());
        });
    }

    void DeleteEntityCommand::undo() {
        const bool wasDeferred = world_.is_deferred();
        if (wasDeferred) world_.defer_suspend();

        // Snapshots are stored parent-first, so ChildOf targets are alive before use.
        for (const auto& snapshot : snapshots_) {
            world_.make_alive(flecs::strip_generation(snapshot.id));
            ecs_set_version(world_, snapshot.id);
            auto entity = world_.entity(snapshot.id);

            if (snapshot.parentId) {
                auto parent = world_.entity(snapshot.parentId);
                if (parent.is_alive()) entity.child_of(parent);
            }
            if (!snapshot.name.empty()) entity.set_name(snapshot.name.c_str());

            if (snapshot.position) entity.set<Position2D>(*snapshot.position);
            if (snapshot.scale) entity.set<Scale2D>(*snapshot.scale);
            if (snapshot.rotation) entity.set<Rotation2D>(*snapshot.rotation);
            if (snapshot.sprite) entity.set<Sprite2D>(*snapshot.sprite);
            if (snapshot.aabb) entity.set<AABBCollider2D>(*snapshot.aabb);
            if (snapshot.circle) entity.set<CircleCollider2D>(*snapshot.circle);
            if (snapshot.camera) entity.set<Camera2D>(*snapshot.camera);

            for (const auto& [schemaId, value] : snapshot.scriptComponents) {
                auto schema = world_.entity(schemaId);
                if (schema.is_alive()) entity.set<ScriptComponent>(schema, value);
            }
        }

        if (wasDeferred) world_.defer_resume();
    }

    void DeleteEntityCommand::redo() {
        if (!rootId_) return;
        auto root = world_.entity(rootId_);
        if (root.is_alive()) root.destruct();
    }

}
