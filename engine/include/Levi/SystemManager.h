#pragma once
#include <string>
#include <vector>
#include <functional>
#include <iostream>
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
            // Check if system already exists to support hot-reload
            for (auto& sys : systems_) {
                if (sys.name == name) {
                    sys.queryComponents = query;
                    sys.callback = callback;
                    std::cout << "[SystemManager] Updated existing system: " << name << std::endl;
                    return;
                }
            }

            LuaSystem system;
            system.name = name;
            system.queryComponents = query;
            system.callback = callback;
            system.isEnabled = true; 
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
