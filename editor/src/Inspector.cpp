#include "Inspector.h"
#include <imgui.h>
#include <imgui_stdlib.h> // For std::string support in InputText
#include "Levi/Components.h"
#include "Levi/ScriptComponent.h"
#include <filesystem>

namespace Levi {

    static void drawAssetPathField(const char* label, std::string& path, const std::string& projectPath) {
        char pathBuf[512];
        strncpy(pathBuf, path.c_str(), sizeof(pathBuf));
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
             ImGui::SetTooltip("Supports absolute paths (converted to relative if inside project) or relative paths to project root.");
        }
    }

    void Inspector::render(flecs::entity entity, const std::string& projectPath) {
        ImGui::Begin("Inspector");

        if (!entity || !entity.is_alive()) {
            ImGui::Text("No entity selected.");
            ImGui::End();
            return;
        }

        // Display Entity Name
        char nameBuf[128];
        strncpy(nameBuf, entity.name().c_str(), sizeof(nameBuf));
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            entity.set_name(nameBuf);
        }

        ImGui::Separator();

        // 1. Position2D Component
        if (entity.has<Position2D>()) {
            if (ImGui::CollapsingHeader("Position 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* pos = entity.get_mut<Position2D>();
                ImGui::DragFloat("X", &pos->x, 1.0f);
                ImGui::DragFloat("Y", &pos->y, 1.0f);
            }
        }

        // 2. Scale2D Component
        if (entity.has<Scale2D>()) {
            if (ImGui::CollapsingHeader("Scale 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* scale = entity.get_mut<Scale2D>();
                ImGui::DragFloat("W", &scale->x, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("H", &scale->y, 0.1f, 0.0f, 100.0f);
            }
        }

        // 3. Rotation2D Component
        if (entity.has<Rotation2D>()) {
            if (ImGui::CollapsingHeader("Rotation 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* rot = entity.get_mut<Rotation2D>();
                ImGui::DragFloat("Angle", &rot->angle, 1.0f, 0.0f, 360.0f);
                
                ImGui::Separator();
                ImGui::Text("Pivot");
                
                // Pivot Type Combo
                const char* pivotTypes[] = { "Percent", "Pixel" };
                int currentType = (int)rot->pivotType;
                if (ImGui::Combo("Pivot Type", &currentType, pivotTypes, IM_ARRAYSIZE(pivotTypes))) {
                    rot->pivotType = (PivotType)currentType;
                }

                if (rot->pivotType == PivotType::Percent) {
                    ImGui::DragFloat2("Pivot (0-1)", &rot->pivot.x, 0.01f, 0.0f, 1.0f);
                } else {
                    ImGui::DragFloat2("Pivot (Px)", &rot->pivot.x, 1.0f);
                }
            }
        }

        // 4. Sprite2D Component
        if (entity.has<Sprite2D>()) {
            if (ImGui::CollapsingHeader("Sprite 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* sprite = entity.get_mut<Sprite2D>();
                
                drawAssetPathField("Texture", sprite->texturePath, projectPath);

                ImGui::DragFloat("Size X", &sprite->size.x, 1.0f, 0.0f, 2048.0f);
                ImGui::DragFloat("Size Y", &sprite->size.y, 1.0f, 0.0f, 2048.0f);
            }
        }

        // --- 5. Dynamic Script Components (Phase 3) ---
        auto& registry = ScriptComponentRegistry::getInstance();
        for (const auto& [schemaName, schema] : registry.getSchemas()) {
            if (entity.has<ScriptComponent>(entity.world().entity(schemaName.c_str()))) {
                if (ImGui::CollapsingHeader(schemaName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    auto* comp = entity.get_mut<ScriptComponent>(entity.world().entity(schemaName.c_str()));
                    
                    for (const auto& field : schema.fields) {
                        auto& value = comp->values[field.name];
                        
                        // If value doesn't exist, initialize with a default based on type
                        if (value.index() == 0 && std::holds_alternative<float>(value) && std::get<float>(value) == 0.0f) {
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
                                std::string val = std::holds_alternative<std::string>(value) ? std::get<std::string>(value) : "";
                                if (ImGui::InputText(field.name.c_str(), &val)) value = val;
                                break;
                            }
                            case ScriptFieldType::AssetPath: {
                                std::string val = std::holds_alternative<std::string>(value) ? std::get<std::string>(value) : "";
                                if (drawAssetPathField(field.name.c_str(), val, projectPath); val != (std::holds_alternative<std::string>(value) ? std::get<std::string>(value) : "")) {
                                    value = val;
                                }
                                break;
                            }
                            case ScriptFieldType::Bool: {
                                bool val = std::holds_alternative<bool>(value) ? std::get<bool>(value) : false;
                                if (ImGui::Checkbox(field.name.c_str(), &val)) value = val;
                                break;
                            }
                            default: break;
                        }
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
                entity.world().defer([entity]() {
                    entity.add<Position2D>();
                });
            }
            if (!entity.has<Scale2D>() && ImGui::MenuItem("Scale 2D")) {
                entity.world().defer([entity]() {
                    entity.add<Scale2D>();
                });
            }
            if (!entity.has<Rotation2D>() && ImGui::MenuItem("Rotation 2D")) {
                entity.world().defer([entity]() {
                    entity.add<Rotation2D>();
                });
            }
            if (!entity.has<Sprite2D>() && ImGui::MenuItem("Sprite 2D")) {
                entity.world().defer([entity]() {
                    entity.add<Sprite2D>();
                });
            }
            
            ImGui::Separator();
            ImGui::TextDisabled("Lua Components");
            for (const auto& [name, schema] : registry.getSchemas()) {
                auto schemaEntity = entity.world().entity(name.c_str());
                if (!entity.has<ScriptComponent>(schemaEntity) && ImGui::MenuItem(name.c_str())) {
                    entity.world().defer([entity, name, schemaEntity]() {
                        ScriptComponent comp;
                        comp.schemaName = name;
                        entity.set<ScriptComponent>(schemaEntity, comp);
                    });
                }
            }
            
            ImGui::EndPopup();
        }

        ImGui::End();
    }

}

