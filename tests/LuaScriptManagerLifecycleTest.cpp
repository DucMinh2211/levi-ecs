#include "Levi/LuaScriptManager.h"
#include "Levi/ScriptComponent.h"
#include "Levi/SystemManager.h"

#include <iostream>

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
