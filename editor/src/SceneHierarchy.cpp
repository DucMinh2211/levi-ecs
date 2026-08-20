#include "SceneHierarchy.h"
#include "DeleteEntityCommand.h"
#include "Levi/ScriptComponent.h"
#include "flecs/addons/cpp/c_types.hpp"
#include <imgui.h>
#include <memory>

namespace Levi {

    namespace {
        bool isVisibleSceneEntity(flecs::entity entity) {
            if (!entity || !entity.is_alive()) return false;
            const std::string path = entity.path().c_str();
            if (path.rfind("::flecs", 0) == 0) return false;

            const std::string name = entity.name().c_str();
            if (name.empty() || name[0] == '$') return false;
            if (entity.has(flecs::Module) || entity.has<flecs::Component>() || entity.has(flecs::System)) return false;
            if (name == "World" || name == "Query" || name == "Observer") return false;

            // Script schemas use Flecs entities as pair targets. They are ECS
            // metadata, not scene objects, so keep them out of the hierarchy.
            return !entity.has<ScriptComponentSchemaTag>()
                && ScriptComponentRegistry::getInstance().getSchema(name) == nullptr;
        }

        std::unique_ptr<EditorCommand> makeCreateEntityCommand(flecs::world world, flecs::entity parent = {}) {
            struct CreatedEntityState {
                CreatedEntityState(flecs::world sourceWorld, flecs::entity_t sourceParent)
                    : world(std::move(sourceWorld)), parentId(sourceParent) {}

                flecs::world world;
                flecs::entity_t parentId;
                flecs::entity_t entityId = 0;
            };

            auto state = std::make_shared<CreatedEntityState>(
                std::move(world), parent ? parent.id() : 0);

            return std::make_unique<LambdaCommand>(
                parent ? "Create Child" : "Create Entity",
                [state]() {
                    if (!state->entityId) return;
                    auto entity = state->world.entity(state->entityId);
                    if (entity.is_alive()) entity.destruct();
                },
                [state]() {
                    auto entity = state->world.entity();
                    if (state->parentId) {
                        auto parent = state->world.entity(state->parentId);
                        if (parent.is_alive()) entity.child_of(parent);
                    } else {
                        entity.set_name("New Entity");
                    }
                    state->entityId = entity.id();
                });
        }
    }

    void SceneHierarchy::render(flecs::world& world, UndoRedoManager& history) {
        history_ = &history;
        ImGui::Begin("Scene Hierarchy");

        // Query tìm các entity gốc
        auto q = world.query_builder()
            .without(flecs::ChildOf, flecs::Wildcard)
            .build();

        q.each([this](flecs::entity e) {
            if (isVisibleSceneEntity(e)) drawEntityNode(e);
        });

        // Right-click on window background
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Create New Entity")) {
                history.execute(makeCreateEntityCommand(world));
            }
            ImGui::EndPopup();
        }

        // Click on empty space to deselect
        if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
            selectedEntity_ = flecs::entity::null();
        }

        ImGui::End();
    }

    void SceneHierarchy::drawEntityNode(flecs::entity e) {
        // Get entity name or ID
        std::string label = e.name().length() > 0 ? e.name().c_str() : "Entity " + std::to_string(e.id());

        ImGuiTreeNodeFlags flags = ((selectedEntity_ == e) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

        // Check if entity has children to decide if it should be a leaf
        bool hasChildren = false;
        e.children([&](flecs::entity child) {
            if (isVisibleSceneEntity(child)) hasChildren = true;
        });

        if (!hasChildren) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)e.id(), flags, "%s", label.c_str());

        if (ImGui::IsItemClicked()) {
            selectedEntity_ = e;
        }

        // Entity context menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Create Child")) {
                history_->execute(makeCreateEntityCommand(e.world(), e));
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Entity")) {
                selectedEntity_ = flecs::entity::null();
                history_->execute(std::make_unique<DeleteEntityCommand>(e));
            }
            ImGui::EndPopup();
        }

        if (opened && hasChildren) {
            e.children([this](flecs::entity child) {
                if (isVisibleSceneEntity(child)) drawEntityNode(child);
            });
            ImGui::TreePop();
        }
    }

}
