#pragma once

#include <any>
#include <flecs.h>
#include <functional>
#include <string>
#include <unordered_map>

#include "UndoRedoManager.h"

namespace Levi {

    class Inspector {
    public:
        Inspector() = default;
        ~Inspector() = default;

        void render(flecs::entity entity, const std::string& projectPath, UndoRedoManager& history);

    private:
        using AnySetter = std::function<void(flecs::entity, const std::any&)>;

        void trackItemEdit(flecs::entity entity, UndoRedoManager& history, const std::string& commandName,
                           std::any valueBeforeWidget, std::any currentValue, AnySetter setter);

        std::unordered_map<unsigned int, std::any> editStartValues_;
    };

} // namespace Levi
