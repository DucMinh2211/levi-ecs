#pragma once

#include <flecs.h>
#include <string>

namespace Levi {

    class SceneHierarchy {
    public:
        SceneHierarchy() = default;
        ~SceneHierarchy() = default;

        // Pass the engine's world to render all entities
        void render(flecs::world& world);

        // Track the currently selected entity for the Inspector
        flecs::entity getSelectedEntity() const { return selectedEntity_; }
        void setSelectedEntity(flecs::entity entity) { selectedEntity_ = entity; }

    private:
        flecs::entity selectedEntity_;
        void drawEntityNode(flecs::entity e);
    };

}
