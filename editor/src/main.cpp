#include <imgui.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <filesystem>

#include "Levi/Engine.h"
#include "ProjectExplorer.h"

int main(int argc, char* argv[]) {
    std::cout << "--- Levi Studio Editor ---" << std::endl;

    Levi::EngineConfig config;
    config.WindowTitle = "Levi Editor v0.1.0";
    config.WindowWidth = 1600;
    config.WindowHeight = 900;

    Levi::EngineCore engine;
    if (engine.init(config)) {
        Levi::ProjectExplorer projectExplorer;

        // --- Test: Create a simple entity ---
        // Resolve the asset path relative to the project root
        std::string playerImagePath = projectExplorer.getCompletePath("assets/player.jpg");

        auto player = engine.getWorld().entity("Player")
            .set<Levi::Position2D>({ 100.0f, 100.0f })
            .set<Levi::Sprite2D>({ playerImagePath, { 300.0f, 300.0f } });

        engine.run([&]() {
            // --- 1. Viewport Window ---
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // Remove window padding
            ImGui::Begin("Viewport");
            
            // Get current ImGui window size to scale the image
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();
            
            // Display Texture from Engine
            SDL_Texture* viewportTex = engine.getViewportTexture();
            if (viewportTex) {
                // In SDL3 + ImGui, we pass the SDL_Texture pointer directly
                ImGui::Image((ImTextureID)viewportTex, viewportSize);
            }
            
            ImGui::End();
            ImGui::PopStyleVar();

            // --- 2. Project Explorer ---
            projectExplorer.render();

            // --- 3. Other Windows ---
            ImGui::Begin("Levi ECS Editor");
            ImGui::Text("Entity: Player");
            ImGui::End();

            ImGui::ShowDemoWindow();
        });
    }

    engine.shutdown();

    return 0;
}
