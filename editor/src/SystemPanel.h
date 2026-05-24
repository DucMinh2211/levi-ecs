#pragma once
#include <imgui.h>
#include "Levi/SystemManager.h"

namespace Levi {

    class SystemPanel {
    public:
        void render() {
            ImGui::Begin("Systems");

            auto& systems = SystemManager::getInstance().getSystems();
            if (systems.empty()) {
                ImGui::TextDisabled("No systems registered via Lua.");
            } else {
                for (auto& sys : systems) {
                    bool enabled = sys.isEnabled;
                    if (ImGui::Checkbox(sys.name.c_str(), &enabled)) {
                        sys.isEnabled = enabled;
                    }
                    if (ImGui::IsItemHovered()) {
                        std::string queryText = "Query: ";
                        for (size_t i = 0; i < sys.queryComponents.size(); ++i) {
                            queryText += sys.queryComponents[i];
                            if (i < sys.queryComponents.size() - 1) queryText += ", ";
                        }
                        ImGui::SetTooltip("%s", queryText.c_str());
                    }
                }
            }

            ImGui::End();
        }
    };

}
