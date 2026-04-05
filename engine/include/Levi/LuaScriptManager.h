#pragma once

#include <sol/sol.hpp>
#include <flecs.h>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>

namespace Levi {
    
    struct ScriptInfo {
        std::string path;
        std::filesystem::file_time_type lastModified;
        bool loaded = false;
        std::string errorMessage;
        
        // Per-script environment and functions
        sol::environment env;
        sol::protected_function onInit;
        sol::protected_function onUpdate;
        sol::protected_function onShutdown;

        // Track entities created by this script for automatic cleanup
        std::vector<uint64_t> createdEntities;
    };

    // API documentation for LSP export
    struct LuaAPIDoc {
        std::string name;
        std::string description;
        std::vector<std::pair<std::string, std::string>> params; // name, type
        std::string returnType;
    };

    class LuaScriptManager {
    public:
        LuaScriptManager();
        ~LuaScriptManager();

        // Initialize Lua VM for a specific project
        bool init(const std::string& projectPath, flecs::world* world);
        
        // Shutdown and cleanup Lua VM
        void shutdown();

        // Bind ECS API to Lua
        void bindECS(flecs::world* world);

        // Load all scripts from project's scripts folder
        bool loadAllScripts();

        // Reload a specific script file
        bool reloadScript(const std::string& scriptPath);

        // Check for file changes and hot-reload
        void checkForChanges();
        
        // Export API definitions for LSP
        void exportAPIDefinitions();

        // Execute Lua function (e.g., onUpdate, onInit)
        template<typename... Args>
        void callFunction(const std::string& functionName, Args&&... args) {
            if (!initialized_) return;
            
            for (auto& scriptInfoPtr : scripts_) {
                ScriptInfo& scriptInfo = *scriptInfoPtr;
                if (!scriptInfo.loaded) continue;
                
                sol::protected_function func;
                if (functionName == "onUpdate") func = scriptInfo.onUpdate;
                else if (functionName == "onInit") func = scriptInfo.onInit;
                else if (functionName == "onShutdown") func = scriptInfo.onShutdown;
                else func = scriptInfo.env[functionName];

                if (func.valid()) {
                    auto result = func(std::forward<Args>(args)...);
                    if (!result.valid()) {
                        sol::error err = result;
                        std::cerr << "[Lua] Error calling " << functionName 
                                  << " in " << scriptInfo.path << ": " << err.what() << std::endl;
                        lastError_ = err.what();
                    }
                }
            }
        }

        // Get Lua state for advanced operations
        sol::state& getLuaState() { return lua_; }

        // Get script info
        const std::vector<std::unique_ptr<ScriptInfo>>& getScripts() const { return scripts_; }
        const std::string& getLastError() const { return lastError_; }
        bool isInitialized() const { return initialized_; }

    private:
        bool loadScript(const std::string& scriptPath);
        void scanScriptsFolder();
        void registerAPIDoc(const LuaAPIDoc& doc);

        sol::state lua_;
        flecs::world* world_ = nullptr;
        std::string projectPath_;
        std::string scriptsFolder_;
        std::vector<std::unique_ptr<ScriptInfo>> scripts_;
        std::vector<LuaAPIDoc> apiDocs_; // Track all registered APIs
        std::string lastError_;
        bool initialized_ = false;
    };

}
