#include "Levi/LuaScriptManager.h"
#include "Levi/ScriptComponent.h"
#include "Levi/SystemManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main() {
    // The world must outlive the Lua manager. This mirrors EngineCore's member
    // order and guards the shutdown sequence that previously asserted in Flecs.
    flecs::world world;

    {
        Levi::LuaScriptManager scripts;

        for (int cycle = 0; cycle < 2; ++cycle) {
            if (!scripts.init(LEVI_TEST_PROJECT_DIR, &world)) {
                std::cerr << "Failed to initialize scripts in cycle " << cycle << '\n';
                return 1;
            }

            sol::table input = scripts.getLuaState()["Input"];
            sol::table physics = scripts.getLuaState()["Physics"];
            sol::table camera = scripts.getLuaState()["Camera"];
            sol::table scene = scripts.getLuaState()["Scene"];
            if (!input.valid() || !input["isKeyDown"].valid() || !physics.valid() || !physics["overlaps"].valid() ||
                !physics["applyImpulse"].valid() || !physics["beganContact"].valid() || !camera.valid() ||
                !camera["move"].valid() || !camera["shake"].valid() || !scene.valid() || !scene["load"].valid() ||
                !scene["reload"].valid()) {
                std::cerr << "Input/Physics/Camera/Scene Lua API was not registered\n";
                return 5;
            }

            const std::filesystem::path projectRoot(LEVI_TEST_PROJECT_DIR);
            const auto projectConfig = projectRoot / ".luarc.json";
            const auto scriptsConfig = projectRoot / "scripts" / ".luarc.json";
            const auto apiDefinitions = projectRoot / "scripts" / "levi-api" / "ecs.lua";
            if (!std::filesystem::exists(projectConfig) || !std::filesystem::exists(scriptsConfig) ||
                !std::filesystem::exists(apiDefinitions)) {
                std::cerr << "LuaLS workspace files were not generated\n";
                return 6;
            }

            std::ifstream apiFile(apiDefinitions, std::ios::binary);
            const std::string apiText{std::istreambuf_iterator<char>(apiFile), std::istreambuf_iterator<char>()};
            if (apiText.find("function ECS.createEntity") == std::string::npos ||
                apiText.find("function Input.isKeyDown") == std::string::npos ||
                apiText.find("function Physics.overlaps") == std::string::npos ||
                apiText.find("function Physics.applyImpulse") == std::string::npos ||
                apiText.find("function ECS.addRigidBody") == std::string::npos ||
                apiText.find("function Camera.shake") == std::string::npos ||
                apiText.find("function Scene.load") == std::string::npos) {
                std::cerr << "Generated LuaLS definitions are incomplete\n";
                return 7;
            }

            if (scripts.getScripts().empty()) {
                std::cerr << "No Lua script was loaded for runtime lifecycle test\n";
                return 8;
            }
            auto& runtimeScript = *scripts.getScripts().front();
            auto defineRuntimeInit = scripts.getLuaState().safe_script(
                "function onInit() ECS.createEntity('__LeviRuntimeLifecycleTest') end", runtimeScript.env);
            if (!defineRuntimeInit.valid()) {
                std::cerr << "Could not define runtime lifecycle fixture\n";
                return 9;
            }
            runtimeScript.onInit = runtimeScript.env["onInit"];

            scripts.startRuntime();
            const auto runtimeEntity = world.lookup("__LeviRuntimeLifecycleTest");
            if (!scripts.isRuntimeActive() || !runtimeEntity || !runtimeEntity.is_alive()) {
                std::cerr << "onInit did not create the runtime entity\n";
                return 10;
            }

            scripts.setCurrentScenePath("scenes/level_1.levscene.json");
            sol::protected_function queueScene = scene["load"];
            auto queued = queueScene("scenes/level_2.levscene.json");
            if (!queued.valid() || !queued.get<bool>() || !scripts.hasPendingSceneLoad()) {
                std::cerr << "Scene.load did not queue a runtime request\n";
                return 12;
            }
            const auto requestedScene = scripts.takePendingSceneLoad();
            if (!requestedScene || *requestedScene != "scenes/level_2.levscene.json" || scripts.hasPendingSceneLoad()) {
                std::cerr << "Queued runtime scene request was not consumed correctly\n";
                return 13;
            }

            scripts.stopRuntime();
            const auto stoppedEntity = world.lookup("__LeviRuntimeLifecycleTest");
            if (scripts.isRuntimeActive() || (stoppedEntity && stoppedEntity.is_alive())) {
                std::cerr << "Runtime entity survived stopRuntime\n";
                return 11;
            }

            scripts.callFunction("onUpdate", 0.016f);
            scripts.shutdown();

            if (scripts.isInitialized()) {
                std::cerr << "Lua manager remained initialized after shutdown\n";
                return 2;
            }
            if (!Levi::SystemManager::getInstance().getSystems().empty()) {
                std::cerr << "Lua callbacks survived VM shutdown\n";
                return 3;
            }
            if (!Levi::ScriptComponentRegistry::getInstance().getSchemas().empty()) {
                std::cerr << "Script schemas survived project shutdown\n";
                return 4;
            }
        }
    }

    return 0;
}
