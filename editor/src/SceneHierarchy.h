#pragma once

#include <flecs.h>
#include <string>

#include "UndoRedoManager.h"

namespace Levi {

    class SceneHierarchy {
    public:
        SceneHierarchy() = default;
        ~SceneHierarchy() = default;

        // Pass the engine's world to render all entities
        void render(flecs::world& world, UndoRedoManager& history);

        // Track the currently selected entity for the Inspector
        flecs::entity getSelectedEntity() const { return selectedEntity_; }
        void setSelectedEntity(flecs::entity entity) { selectedEntity_ = entity; }

    private:
        flecs::entity selectedEntity_;
        UndoRedoManager* history_ = nullptr;
        void drawEntityNode(flecs::entity e);
    };

}
