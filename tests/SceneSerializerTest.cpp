#include "Levi/SceneSerializer.h"
#include "Levi/Components.h"
#include "Levi/ScriptComponent.h"

#include <string>

namespace {
    bool verify(flecs::world& world) {
        auto root = world.lookup("Root");
        if (!root.is_alive()) return false;
        auto child = root.lookup("Child");
        if (!child.is_alive() || child.parent() != root) return false;
        const auto* position = root.get<Levi::Position2D>();
        const auto* sprite = root.get<Levi::Sprite2D>();
        const auto* collider = child.get<Levi::CircleCollider2D>();
        const auto* rigidBody = child.get<Levi::RigidBody2D>();
        const auto* camera = root.get<Levi::Camera2D>();
        const auto* script = child.get<Levi::ScriptComponent>(world.lookup("Health"));
        int unnamedPositions = 0;
        world.each<Levi::Position2D>([&unnamedPositions](flecs::entity entity, Levi::Position2D& value) {
            if (entity.name().length() == 0 && value.x == 3.0f && value.y == 4.0f) ++unnamedPositions;
        });
        return position && position->x == 10.0f && position->y == 20.0f && sprite &&
               sprite->texturePath == "assets/quoted\"name.png" && collider && collider->radius == 12.5f && rigidBody &&
               rigidBody->type == Levi::RigidBodyType2D::Kinematic && rigidBody->linearVelocity.x == 120.0f &&
               rigidBody->gravityScale == 0.25f && rigidBody->sensor && rigidBody->maskBits == 0x0Fu && camera &&
               camera->zoom == 2.0f && camera->active && camera->maxShakeOffset == 20.0f &&
               camera->shakeRemaining == 0.0f && script && std::get<int>(script->values.at("hp")) == 80 &&
               std::get<bool>(script->values.at("active")) && unnamedPositions == 1;
    }
} // namespace

int main() {
    flecs::world world;
    Levi::ScriptComponentRegistry::getInstance().registerSchema({"Health", {{"hp", Levi::ScriptFieldType::Int}}});
    auto schema = world.entity("Health").add<Levi::ScriptComponentSchemaTag>();
    auto root = world.entity("Root");
    auto child = world.entity("Child").child_of(root);
    root.set<Levi::Position2D>({10.0f, 20.0f});
    root.set<Levi::Sprite2D>({"assets/quoted\"name.png", {32.0f, 48.0f}});
    Levi::Camera2D camera;
    camera.zoom = 2.0f;
    camera.maxShakeOffset = 20.0f;
    camera.maxShakeRotation = 4.0f;
    camera.shakeIntensity = 1.0f;
    camera.shakeDuration = 2.0f;
    camera.shakeRemaining = 2.0f;
    root.set<Levi::Camera2D>(camera);
    child.set<Levi::CircleCollider2D>({12.5f, {1.0f, 2.0f}});
    Levi::RigidBody2D rigidBody;
    rigidBody.type = Levi::RigidBodyType2D::Kinematic;
    rigidBody.linearVelocity = {120.0f, -40.0f};
    rigidBody.gravityScale = 0.25f;
    rigidBody.sensor = true;
    rigidBody.maskBits = 0x0Fu;
    child.set<Levi::RigidBody2D>(rigidBody);
    Levi::ScriptComponent health;
    health.schemaName = "Health";
    health.values["hp"] = 80;
    health.values["active"] = true;
    child.set<Levi::ScriptComponent>(schema, health);
    world.entity().set<Levi::Position2D>({3.0f, 4.0f});

    const std::string json = Levi::SceneSerializer::toJson(world);
    if (json.find("levi.scene") == std::string::npos || json.find("Root") == std::string::npos) return 1;
    root.set<Levi::Position2D>({999.0f, 999.0f});
    world.entity().set<Levi::Position2D>({7.0f, 8.0f});
    std::string error;
    if (!Levi::SceneSerializer::fromJson(world, json, &error) || !verify(world)) return 2;

    const auto binary = Levi::SceneSerializer::toBinary(world);
    world.lookup("Root").destruct();
    if (!Levi::SceneSerializer::fromBinary(world, binary, &error) || !verify(world)) return 3;

    if (Levi::SceneSerializer::fromJson(world, "{}", &error) || error.empty()) return 4;
    Levi::ScriptComponentRegistry::getInstance().clear();
    return 0;
}
