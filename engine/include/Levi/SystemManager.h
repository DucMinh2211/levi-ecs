#pragma once
#include <string>
#include <vector>
#include <functional>
#include <sol/sol.hpp>

namespace Levi {

    struct LuaSystem {
        std::string name;
        std::vector<std::string> queryComponents;
        sol::protected_function callback;
        bool isEnabled = true;
    };

    class SystemManager {
    public:
        static SystemManager& getInstance() {
            static SystemManager instance;
            return instance;
        }

        void registerSystem(const std::string& name, const std::vector<std::string>& query, sol::protected_function callback) {
            LuaSystem system;
            system.name = name;
            system.queryComponents = query;
            system.callback = callback;
            system.isEnabled = true; // Default to enabled for now, editor will control this
            systems_.push_back(system);
        }

        std::vector<LuaSystem>& getSystems() {
            return systems_;
        }

        void clear() {
            systems_.clear();
        }

    private:
        SystemManager() = default;
        std::vector<LuaSystem> systems_;
    };

}
