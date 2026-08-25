#include "DeleteEntityCommand.h"
#include "UndoRedoManager.h"

#include "Levi/Components.h"
#include "Levi/ScriptComponent.h"

#include <memory>
#include <string>

int main() {
    flecs::world world;
    Levi::UndoRedoManager history;

    auto schema = world.entity("Health");
    auto externalParent = world.entity("Scene");
    auto root = world.entity("Player").child_of(externalParent);
    auto child = world.entity("Weapon").child_of(root);
    auto grandchild = world.entity("Effect").child_of(child);

    root.set<Levi::Position2D>({12.0f, 34.0f});
    root.set<Levi::Sprite2D>({"assets/player.png", {48.0f, 64.0f}});
    Levi::Camera2D camera;
    camera.zoom = 1.5f;
    camera.maxShakeOffset = 18.0f;
    root.set<Levi::Camera2D>(camera);
    child.set<Levi::Scale2D>({2.0f, 3.0f});
    Levi::RigidBody2D rigidBody;
    rigidBody.type = Levi::RigidBodyType2D::Dynamic;
    rigidBody.restitution = 0.75f;
    child.set<Levi::RigidBody2D>(rigidBody);
    grandchild.set<Levi::Rotation2D>({45.0f, {0.25f, 0.75f}, Levi::PivotType::Percent});

    Levi::ScriptComponent health;
    health.schemaName = "Health";
    health.values["current"] = 75;
    health.values["label"] = std::string("Player HP");
    child.set<Levi::ScriptComponent>(schema, health);

    const auto rootId = root.id();
    const auto childId = child.id();
    const auto grandchildId = grandchild.id();

    history.execute(std::make_unique<Levi::DeleteEntityCommand>(root));
    if (world.entity(rootId).is_alive()) return 1;
    if (world.entity(childId).is_alive()) return 2;
    if (world.entity(grandchildId).is_alive()) return 3;

    if (!history.undo()) return 4;
    root = world.entity(rootId);
    child = world.entity(childId);
    grandchild = world.entity(grandchildId);
    if (!root.is_alive() || !child.is_alive() || !grandchild.is_alive()) return 5;
    if (root.parent() != externalParent || child.parent() != root || grandchild.parent() != child) return 6;
    if (std::string(root.name().c_str()) != "Player" || std::string(child.name().c_str()) != "Weapon") return 7;

    const auto* position = root.get<Levi::Position2D>();
    const auto* sprite = root.get<Levi::Sprite2D>();
    const auto* restoredCamera = root.get<Levi::Camera2D>();
    const auto* scale = child.get<Levi::Scale2D>();
    const auto* restoredBody = child.get<Levi::RigidBody2D>();
    const auto* rotation = grandchild.get<Levi::Rotation2D>();
    const auto* restoredHealth = child.get<Levi::ScriptComponent>(schema);
    if (!position || position->x != 12.0f || position->y != 34.0f) return 8;
    if (!sprite || sprite->texturePath != "assets/player.png" || sprite->size.x != 48.0f) return 9;
    if (!restoredCamera || restoredCamera->zoom != 1.5f || restoredCamera->maxShakeOffset != 18.0f) return 22;
    if (!scale || scale->x != 2.0f || scale->y != 3.0f) return 10;
    if (!restoredBody || restoredBody->type != Levi::RigidBodyType2D::Dynamic || restoredBody->restitution != 0.75f)
        return 23;
    if (!rotation || rotation->angle != 45.0f || rotation->pivot.x != 0.25f) return 11;
    if (!restoredHealth || std::get<int>(restoredHealth->values.at("current")) != 75) return 12;
    if (std::get<std::string>(restoredHealth->values.at("label")) != "Player HP") return 13;

    if (!history.redo()) return 14;
    if (world.entity(rootId).is_alive() || world.entity(childId).is_alive()) return 15;

    if (!history.undo()) return 16;
    if (!world.entity(rootId).is_alive() || !world.entity(grandchildId).is_alive()) return 17;

    // The editor wraps UI callbacks in a global deferred scope.
    world.defer_begin();
    if (!history.redo()) return 18;
    world.defer_end();
    if (world.entity(rootId).is_alive()) return 19;

    world.defer_begin();
    if (!history.undo()) return 20;
    world.defer_end();
    if (!world.entity(rootId).is_alive() || !world.entity(grandchildId).is_alive()) return 21;
    return 0;
}
