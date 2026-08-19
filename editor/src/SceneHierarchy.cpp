#include "SceneHierarchy.h"
#include "flecs/addons/cpp/c_types.hpp"
#include <imgui.h>
#include <memory>

namespace Levi {

    namespace {
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
            .without(flecs::ChildOf)
            .build();

        q.each([this](flecs::entity e) {
            // 1. Kiểm tra đường dẫn đầy đủ (Full Path)
            // Đây là cách cực kỳ hiệu quả: Các thực thể hệ thống sẽ có path như "::flecs::core::World"
            std::string path = e.path().c_str();
            if (path.find("::flecs") == 0) {
                return;
            }

            // 2. Lọc bỏ các thực thể không có tên hoặc là tiền tố đặc biệt
            const char* name = e.name().c_str();
            if (!name || name[0] == '\0' || name[0] == '$') {
                return;
            }

            // 3. Lọc bổ sung các thực thể nội bộ khác (nếu còn sót)
            if (e.has(flecs::Module) || e.has<flecs::Component>() || e.has(flecs::System)) {
                return;
            }

            // 4. Loại bỏ một số từ khóa phổ biến của Flecs không nằm trong namespace flecs (nếu có)
            if (strcmp(name, "World") == 0 || strcmp(name, "Query") == 0 || strcmp(name, "Observer") == 0) {
                return;
            }

            drawEntityNode(e);
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
            hasChildren = true;
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
                e.world().defer([this, e]() {
                    if (selectedEntity_ == e) selectedEntity_ = flecs::entity::null();
                    e.destruct();
                });
            }
            ImGui::EndPopup();
        }

        if (opened && hasChildren) {
            e.children([this](flecs::entity child) {
                drawEntityNode(child);
            });
            ImGui::TreePop();
        }
    }

}
