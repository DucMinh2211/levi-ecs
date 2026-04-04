#include "Inspector.h"
#include <imgui.h>
#include "Levi/Components.h"

namespace Levi {

    void Inspector::render(flecs::entity entity) {
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

        // 3. Sprite2D Component
        if (entity.has<Sprite2D>()) {
            if (ImGui::CollapsingHeader("Sprite 2D", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto* sprite = entity.get_mut<Sprite2D>();
                
                char pathBuf[512];
                strncpy(pathBuf, sprite->texturePath.c_str(), sizeof(pathBuf));
                if (ImGui::InputText("Texture", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                    sprite->texturePath = pathBuf;
                }

                ImGui::DragFloat("Size X", &sprite->size.x, 1.0f, 0.0f, 2048.0f);
                ImGui::DragFloat("Size Y", &sprite->size.y, 1.0f, 0.0f, 2048.0f);
            }
        }

        // --- Button to add components ---
        ImGui::Separator();
        if (ImGui::Button("Add Component")) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup")) {
            if (!entity.has<Position2D>() && ImGui::MenuItem("Position 2D")) {
                entity.add<Position2D>();
            }
            if (!entity.has<Scale2D>() && ImGui::MenuItem("Scale 2D")) {
                entity.add<Scale2D>();
            }
            if (!entity.has<Sprite2D>() && ImGui::MenuItem("Sprite 2D")) {
                entity.add<Sprite2D>();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
    }

}
