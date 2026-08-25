#include "Inspector.h"
#include "Levi/Camera2D.h"
#include "Levi/Components.h"
#include "Levi/ScriptComponent.h"
#include <algorithm>
#include <filesystem>
#include <imgui.h>
#include <imgui_stdlib.h> // For std::string support in InputText
#include <memory>

namespace Levi {

    static void drawAssetPathField(const char* label, std::string& path, const std::string& projectPath) {
        char pathBuf[512];
        strncpy(pathBuf, path.c_str(), sizeof(pathBuf) - 1);
        pathBuf[sizeof(pathBuf) - 1] = '\0';
        if (ImGui::InputText(label, pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            std::filesystem::path newPath(pathBuf);
            if (!projectPath.empty() && newPath.is_absolute()) {
                // Try to make it relative if it's within the project
                try {
                    std::filesystem::path absProj = std::filesystem::absolute(projectPath);
                    if (newPath.string().find(absProj.string()) == 0) {
                        path = std::filesystem::relative(newPath, absProj).string();
                    } else {
                        path = pathBuf;
                    }
                } catch (...) {
                    path = pathBuf;
                }
            } else {
                path = pathBuf;
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "Supports absolute paths (converted to relative if inside project) or relative paths to project root.");
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LEVI_ASSET_PATH")) {
                path = static_cast<const char*>(payload->Data);
            }
            ImGui::EndDragDropTarget();
        }
    }

    void Inspector::trackItemEdit(flecs::entity entity, UndoRedoManager& history, const std::string& commandName,
                                  std::any valueBeforeWidget, std::any currentValue, AnySetter setter) {
        const unsigned int itemId = ImGui::GetItemID();
        if (ImGui::IsItemActivated()) {
            editStartValues_[itemId] = std::move(valueBeforeWidget);
        }

        if (!ImGui::IsItemDeactivatedAfterEdit()) return;

        auto start = editStartValues_.find(itemId);
        if (start == editStartValues_.end()) return;

        std::any previousValue = std::move(start->second);
        editStartValues_.erase(start);
        history.record(std::make_unique<LambdaCommand>(
            commandName,
            [entity, previousValue, setter]() {
                if (entity.is_alive()) setter(entity, previousValue);
            },
            [entity, currentValue, setter]() {
                if (entity.is_alive()) setter(entity, currentValue);
            }));
    }

    void Inspector::render(flecs::entity entity, const std::string& projectPath, UndoRedoManager& history) {
        ImGui::Begin("Inspector");

        if (!entity || !entity.is_alive()) {
            ImGui::Text("No entity selected.");
            ImGui::End();
            return;
        }

        // Display Entity Name
        char nameBuf[128];
        const std::string nameBefore = entity.name().c_str();
        strncpy(nameBuf, nameBefore.c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            entity.set_name(nameBuf);
        }
        trackItemEdit(entity, history, "Rename Entity", nameBefore, std::string(entity.name().c_str()),
                      [](flecs::entity target, const std::any& value) {
                          target.set_name(std::any_cast<const std::string&>(value).c_str());
                      });

        ImGui::Separator();

        // 1. Position2D Component
        if (entity.has<Position2D>()) {
            if (ImGui::CollapsingHeader("Position 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* pos = entity.get_mut<Position2D>();
                Position2D before = *pos;
                ImGui::DragFloat("X", &pos->x, 1.0f);
                trackItemEdit(entity, history, "Edit Position X", before, *pos,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Position2D>(std::any_cast<const Position2D&>(value));
                              });
                before = *pos;
                ImGui::DragFloat("Y", &pos->y, 1.0f);
                trackItemEdit(entity, history, "Edit Position Y", before, *pos,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Position2D>(std::any_cast<const Position2D&>(value));
                              });
            }
        }

        // 2. Scale2D Component
        if (entity.has<Scale2D>()) {
            if (ImGui::CollapsingHeader("Scale 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* scale = entity.get_mut<Scale2D>();
                Scale2D before = *scale;
                ImGui::DragFloat("W", &scale->x, 0.1f, 0.0f, 100.0f);
                trackItemEdit(entity, history, "Edit Scale W", before, *scale,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Scale2D>(std::any_cast<const Scale2D&>(value));
                              });
                before = *scale;
                ImGui::DragFloat("H", &scale->y, 0.1f, 0.0f, 100.0f);
                trackItemEdit(entity, history, "Edit Scale H", before, *scale,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Scale2D>(std::any_cast<const Scale2D&>(value));
                              });
            }
        }

        // 3. Rotation2D Component
        if (entity.has<Rotation2D>()) {
            if (ImGui::CollapsingHeader("Rotation 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* rot = entity.get_mut<Rotation2D>();
                Rotation2D before = *rot;
                ImGui::DragFloat("Angle", &rot->angle, 1.0f, 0.0f, 360.0f);
                trackItemEdit(entity, history, "Edit Rotation", before, *rot,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Rotation2D>(std::any_cast<const Rotation2D&>(value));
                              });

                ImGui::Separator();
                ImGui::Text("Pivot");

                // Pivot Type Combo
                const char* pivotTypes[] = {"Percent", "Pixel"};
                int currentType = (int)rot->pivotType;
                before = *rot;
                if (ImGui::Combo("Pivot Type", &currentType, pivotTypes, IM_ARRAYSIZE(pivotTypes))) {
                    rot->pivotType = (PivotType)currentType;
                }
                trackItemEdit(entity, history, "Edit Pivot Type", before, *rot,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Rotation2D>(std::any_cast<const Rotation2D&>(value));
                              });

                before = *rot;
                if (rot->pivotType == PivotType::Percent) {
                    ImGui::DragFloat2("Pivot (0-1)", &rot->pivot.x, 0.01f, 0.0f, 1.0f);
                } else {
                    ImGui::DragFloat2("Pivot (Px)", &rot->pivot.x, 1.0f);
                }
                trackItemEdit(entity, history, "Edit Pivot", before, *rot,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Rotation2D>(std::any_cast<const Rotation2D&>(value));
                              });
            }
        }

        // 4. Sprite2D Component
        if (entity.has<Sprite2D>()) {
            if (ImGui::CollapsingHeader("Sprite 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* sprite = entity.get_mut<Sprite2D>();
                Sprite2D before = *sprite;
                drawAssetPathField("Texture", sprite->texturePath, projectPath);
                trackItemEdit(entity, history, "Edit Sprite Texture", before, *sprite,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Sprite2D>(std::any_cast<const Sprite2D&>(value));
                              });

                before = *sprite;
                ImGui::DragFloat("Size X", &sprite->size.x, 1.0f, 0.0f, 2048.0f);
                trackItemEdit(entity, history, "Edit Sprite Width", before, *sprite,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Sprite2D>(std::any_cast<const Sprite2D&>(value));
                              });
                before = *sprite;
                ImGui::DragFloat("Size Y", &sprite->size.y, 1.0f, 0.0f, 2048.0f);
                trackItemEdit(entity, history, "Edit Sprite Height", before, *sprite,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Sprite2D>(std::any_cast<const Sprite2D&>(value));
                              });
            }
        }

        if (entity.has<AABBCollider2D>()) {
            if (ImGui::CollapsingHeader("AABB Collider 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* collider = entity.get_mut<AABBCollider2D>();
                AABBCollider2D before = *collider;
                ImGui::DragFloat2("Size##AABB", &collider->size.x, 1.0f, 0.0f);
                trackItemEdit(entity, history, "Edit AABB Size", before, *collider,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<AABBCollider2D>(std::any_cast<const AABBCollider2D&>(value));
                              });
                before = *collider;
                ImGui::DragFloat2("Offset##AABB", &collider->offset.x, 1.0f);
                trackItemEdit(entity, history, "Edit AABB Offset", before, *collider,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<AABBCollider2D>(std::any_cast<const AABBCollider2D&>(value));
                              });
            }
        }

        if (entity.has<CircleCollider2D>()) {
            if (ImGui::CollapsingHeader("Circle Collider 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* collider = entity.get_mut<CircleCollider2D>();
                CircleCollider2D before = *collider;
                ImGui::DragFloat("Radius", &collider->radius, 1.0f, 0.0f);
                trackItemEdit(entity, history, "Edit Circle Radius", before, *collider,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<CircleCollider2D>(std::any_cast<const CircleCollider2D&>(value));
                              });
                before = *collider;
                ImGui::DragFloat2("Offset##Circle", &collider->offset.x, 1.0f);
                trackItemEdit(entity, history, "Edit Circle Offset", before, *collider,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<CircleCollider2D>(std::any_cast<const CircleCollider2D&>(value));
                              });
            }
        }

        if (entity.has<RigidBody2D>()) {
            if (ImGui::CollapsingHeader("Rigid Body 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* body = entity.get_mut<RigidBody2D>();
                auto recordBodyEdit = [this, entity, &history](const char* name, const RigidBody2D& before,
                                                               const RigidBody2D& after) {
                    trackItemEdit(entity, history, name, before, after,
                                  [](flecs::entity target, const std::any& value) {
                                      target.set<RigidBody2D>(std::any_cast<const RigidBody2D&>(value));
                                  });
                };

                const char* bodyTypes[] = {"Static", "Kinematic", "Dynamic"};
                int type = static_cast<int>(body->type);
                RigidBody2D before = *body;
                if (ImGui::Combo("Body Type", &type, bodyTypes, IM_ARRAYSIZE(bodyTypes))) {
                    body->type = static_cast<RigidBodyType2D>(type);
                }
                recordBodyEdit("Edit Body Type", before, *body);

                before = *body;
                ImGui::DragFloat2("Linear Velocity (px/s)", &body->linearVelocity.x, 1.0f);
                recordBodyEdit("Edit Linear Velocity", before, *body);
                before = *body;
                ImGui::DragFloat("Angular Velocity (deg/s)", &body->angularVelocity, 1.0f);
                recordBodyEdit("Edit Angular Velocity", before, *body);
                before = *body;
                ImGui::DragFloat("Gravity Scale", &body->gravityScale, 0.05f);
                recordBodyEdit("Edit Gravity Scale", before, *body);

                before = *body;
                if (ImGui::DragFloat("Linear Damping", &body->linearDamping, 0.05f, 0.0f, 100.0f)) {
                    body->linearDamping = std::max(0.0f, body->linearDamping);
                }
                recordBodyEdit("Edit Linear Damping", before, *body);
                before = *body;
                if (ImGui::DragFloat("Angular Damping", &body->angularDamping, 0.05f, 0.0f, 100.0f)) {
                    body->angularDamping = std::max(0.0f, body->angularDamping);
                }
                recordBodyEdit("Edit Angular Damping", before, *body);

                ImGui::SeparatorText("Material");
                before = *body;
                if (ImGui::DragFloat("Density", &body->density, 0.05f, 0.0f, 1000.0f)) {
                    body->density = std::max(0.0f, body->density);
                }
                recordBodyEdit("Edit Density", before, *body);
                before = *body;
                if (ImGui::SliderFloat("Friction", &body->friction, 0.0f, 1.0f)) {
                    body->friction = std::clamp(body->friction, 0.0f, 1.0f);
                }
                recordBodyEdit("Edit Friction", before, *body);
                before = *body;
                if (ImGui::SliderFloat("Restitution", &body->restitution, 0.0f, 1.0f)) {
                    body->restitution = std::clamp(body->restitution, 0.0f, 1.0f);
                }
                recordBodyEdit("Edit Restitution", before, *body);

                before = *body;
                ImGui::Checkbox("Fixed Rotation", &body->fixedRotation);
                recordBodyEdit("Toggle Fixed Rotation", before, *body);
                before = *body;
                ImGui::Checkbox("Continuous (Bullet)", &body->bullet);
                recordBodyEdit("Toggle Bullet", before, *body);
                before = *body;
                ImGui::Checkbox("Enabled", &body->enabled);
                recordBodyEdit("Toggle Body Enabled", before, *body);
                before = *body;
                ImGui::Checkbox("Sensor", &body->sensor);
                recordBodyEdit("Toggle Sensor", before, *body);

                ImGui::SeparatorText("Collision Filter");
                before = *body;
                ImGui::InputScalar("Category Bits", ImGuiDataType_U32, &body->categoryBits);
                recordBodyEdit("Edit Category Bits", before, *body);
                before = *body;
                ImGui::InputScalar("Mask Bits", ImGuiDataType_U32, &body->maskBits);
                recordBodyEdit("Edit Mask Bits", before, *body);

                if (!entity.has<AABBCollider2D>() && !entity.has<CircleCollider2D>()) {
                    ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.2f, 1.0f),
                                       "Add an AABB or Circle Collider to simulate this body.");
                }
            }
        }

        if (entity.has<Camera2D>()) {
            if (ImGui::CollapsingHeader("Camera 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* camera = entity.get_mut<Camera2D>();
                Camera2D before = *camera;
                if (ImGui::DragFloat("Zoom##Camera", &camera->zoom, 0.05f, Camera2DSystem::MinZoom,
                                     Camera2DSystem::MaxZoom)) {
                    camera->zoom = Camera2DSystem::clampZoom(camera->zoom);
                }
                trackItemEdit(entity, history, "Edit Camera Zoom", before, *camera,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Camera2D>(std::any_cast<const Camera2D&>(value));
                              });

                before = *camera;
                ImGui::Checkbox("Active Camera", &camera->active);
                trackItemEdit(entity, history, "Toggle Active Camera", before, *camera,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Camera2D>(std::any_cast<const Camera2D&>(value));
                              });

                before = *camera;
                ImGui::DragFloat("Max Shake Offset", &camera->maxShakeOffset, 0.25f, 0.0f, 1000.0f);
                trackItemEdit(entity, history, "Edit Camera Shake Offset", before, *camera,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Camera2D>(std::any_cast<const Camera2D&>(value));
                              });

                before = *camera;
                ImGui::DragFloat("Max Shake Rotation", &camera->maxShakeRotation, 0.1f, 0.0f, 180.0f);
                trackItemEdit(entity, history, "Edit Camera Shake Rotation", before, *camera,
                              [](flecs::entity target, const std::any& value) {
                                  target.set<Camera2D>(std::any_cast<const Camera2D&>(value));
                              });

                if (camera->shakeRemaining > 0.0f) {
                    ImGui::TextDisabled("Shake remaining: %.2fs", camera->shakeRemaining);
                }
            }
        }

        // --- Dynamic Script Components ---
        auto& registry = ScriptComponentRegistry::getInstance();
        for (const auto& [schemaName, schema] : registry.getSchemas()) {
            if (entity.has<ScriptComponent>(entity.world().entity(schemaName.c_str()))) {
                if (ImGui::CollapsingHeader(schemaName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto* comp = entity.get_mut<ScriptComponent>(entity.world().entity(schemaName.c_str()));

                    for (const auto& field : schema.fields) {
                        ScriptComponent componentBefore = *comp;
                        auto& value = comp->values[field.name];

                        // If value doesn't exist, initialize with a default based on type
                        if (value.index() == 0 && std::holds_alternative<float>(value) &&
                            std::get<float>(value) == 0.0f) {
                            // std::variant default-initializes the first type.
                            // We might need to handle this more robustly if the first type isn't what we want.
                        }

                        switch (field.type) {
                        case ScriptFieldType::Float: {
                            float val = std::holds_alternative<float>(value) ? std::get<float>(value) : 0.0f;
                            if (ImGui::DragFloat(field.name.c_str(), &val)) value = val;
                            break;
                        }
                        case ScriptFieldType::Int: {
                            int val = std::holds_alternative<int>(value) ? std::get<int>(value) : 0;
                            if (ImGui::DragInt(field.name.c_str(), &val)) value = val;
                            break;
                        }
                        case ScriptFieldType::String: {
                            std::string val =
                                std::holds_alternative<std::string>(value) ? std::get<std::string>(value) : "";
                            if (ImGui::InputText(field.name.c_str(), &val)) value = val;
                            break;
                        }
                        case ScriptFieldType::AssetPath: {
                            std::string val =
                                std::holds_alternative<std::string>(value) ? std::get<std::string>(value) : "";
                            if (drawAssetPathField(field.name.c_str(), val, projectPath);
                                val !=
                                (std::holds_alternative<std::string>(value) ? std::get<std::string>(value) : "")) {
                                value = val;
                            }
                            break;
                        }
                        case ScriptFieldType::Bool: {
                            bool val = std::holds_alternative<bool>(value) ? std::get<bool>(value) : false;
                            if (ImGui::Checkbox(field.name.c_str(), &val)) value = val;
                            break;
                        }
                        default:
                            break;
                        }

                        trackItemEdit(entity, history, "Edit " + schemaName + "." + field.name, componentBefore, *comp,
                                      [schemaName](flecs::entity target, const std::any& value) {
                                          target.set<ScriptComponent>(target.world().entity(schemaName.c_str()),
                                                                      std::any_cast<const ScriptComponent&>(value));
                                      });
                    }
                }
            }
        }

        // --- Button to add components ---
        ImGui::Separator();
        if (ImGui::Button("Add Component", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup")) {
            if (!entity.has<Position2D>() && ImGui::MenuItem("Position 2D")) {
                history.execute(std::make_unique<LambdaCommand>(
                    "Add Position 2D",
                    [entity]() {
                        if (entity.is_alive()) entity.remove<Position2D>();
                    },
                    [entity]() {
                        if (entity.is_alive()) entity.add<Position2D>();
                    }));
            }
            if (!entity.has<Scale2D>() && ImGui::MenuItem("Scale 2D")) {
                history.execute(std::make_unique<LambdaCommand>(
                    "Add Scale 2D",
                    [entity]() {
                        if (entity.is_alive()) entity.remove<Scale2D>();
                    },
                    [entity]() {
                        if (entity.is_alive()) entity.add<Scale2D>();
                    }));
            }
            if (!entity.has<Rotation2D>() && ImGui::MenuItem("Rotation 2D")) {
                history.execute(std::make_unique<LambdaCommand>(
                    "Add Rotation 2D",
                    [entity]() {
                        if (entity.is_alive()) entity.remove<Rotation2D>();
                    },
                    [entity]() {
                        if (entity.is_alive()) entity.add<Rotation2D>();
                    }));
            }
            if (!entity.has<Sprite2D>() && ImGui::MenuItem("Sprite 2D")) {
                history.execute(std::make_unique<LambdaCommand>(
                    "Add Sprite 2D",
                    [entity]() {
                        if (entity.is_alive()) entity.remove<Sprite2D>();
                    },
                    [entity]() {
                        if (entity.is_alive()) entity.add<Sprite2D>();
                    }));
            }
            if (!entity.has<AABBCollider2D>() && ImGui::MenuItem("AABB Collider 2D")) {
                history.execute(std::make_unique<LambdaCommand>(
                    "Add AABB Collider 2D",
                    [entity]() {
                        if (entity.is_alive()) entity.remove<AABBCollider2D>();
                    },
                    [entity]() {
                        if (entity.is_alive()) entity.add<AABBCollider2D>();
                    }));
            }
            if (!entity.has<CircleCollider2D>() && ImGui::MenuItem("Circle Collider 2D")) {
                history.execute(std::make_unique<LambdaCommand>(
                    "Add Circle Collider 2D",
                    [entity]() {
                        if (entity.is_alive()) entity.remove<CircleCollider2D>();
                    },
                    [entity]() {
                        if (entity.is_alive()) entity.add<CircleCollider2D>();
                    }));
            }
            if (!entity.has<RigidBody2D>() && ImGui::MenuItem("Rigid Body 2D")) {
                const bool hadPosition = entity.has<Position2D>();
                history.execute(std::make_unique<LambdaCommand>(
                    "Add Rigid Body 2D",
                    [entity, hadPosition]() {
                        if (!entity.is_alive()) return;
                        entity.remove<RigidBody2D>();
                        if (!hadPosition) entity.remove<Position2D>();
                    },
                    [entity]() {
                        if (!entity.is_alive()) return;
                        if (!entity.has<Position2D>()) entity.set<Position2D>({0.0f, 0.0f});
                        entity.set<RigidBody2D>({});
                    }));
            }
            if (!entity.has<Camera2D>() && ImGui::MenuItem("Camera 2D")) {
                const bool hadPosition = entity.has<Position2D>();
                history.execute(std::make_unique<LambdaCommand>(
                    "Add Camera 2D",
                    [entity, hadPosition]() {
                        if (!entity.is_alive()) return;
                        entity.remove<Camera2D>();
                        if (!hadPosition) entity.remove<Position2D>();
                    },
                    [entity]() {
                        if (!entity.is_alive()) return;
                        if (!entity.has<Position2D>()) entity.set<Position2D>({0.0f, 0.0f});
                        entity.set<Camera2D>({});
                    }));
            }

            ImGui::Separator();
            ImGui::TextDisabled("Lua Components");
            for (const auto& [name, schema] : registry.getSchemas()) {
                auto schemaEntity = entity.world().entity(name.c_str());
                if (!entity.has<ScriptComponent>(schemaEntity) && ImGui::MenuItem(name.c_str())) {
                    history.execute(std::make_unique<LambdaCommand>(
                        "Add " + name,
                        [entity, name]() {
                            if (entity.is_alive()) entity.remove<ScriptComponent>(entity.world().entity(name.c_str()));
                        },
                        [entity, name]() {
                            if (!entity.is_alive()) return;
                            ScriptComponent comp;
                            comp.schemaName = name;
                            entity.set<ScriptComponent>(entity.world().entity(name.c_str()), comp);
                        }));
                }
            }

            ImGui::EndPopup();
        }

        ImGui::End();
    }

} // namespace Levi
