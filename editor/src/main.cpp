#include <imgui.h>
#include <imgui_internal.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <filesystem>
#include <nfd.hpp>

#include "Levi/Engine.h"
#include "ProjectExplorer.h"
#include "SceneHierarchy.h"
#include "Inspector.h"

int main(int argc, char* argv[]) {
    std::cout << "--- Levi Studio Editor ---" << std::endl;

    Levi::EngineConfig config;
    config.WindowTitle = "Levi Editor v0.1.0";
    config.WindowWidth = 1600;
    config.WindowHeight = 900;

    Levi::EngineCore engine;
    if (engine.init(config)) {
        Levi::ProjectExplorer projectExplorer;
        Levi::SceneHierarchy sceneHierarchy;
        Levi::Inspector inspector;

        // --- Logic quản lý đường dẫn imgui.ini ---
        static char iniPathBuf[1024];
        auto updateIniPath = [&](const std::string& projectPath) {
            std::filesystem::path configDir = std::filesystem::path(projectPath) / "editor-configs";
            std::filesystem::create_directories(configDir); // Tạo folder nếu chưa có
            
            std::string iniPath = (configDir / "imgui.ini").make_preferred().string();
            strncpy(iniPathBuf, iniPath.c_str(), sizeof(iniPathBuf));
            
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = iniPathBuf; // Trỏ ImGui vào file mới
            
            // Load lại settings từ file mới nếu nó đã tồn tại
            if (std::filesystem::exists(iniPath)) {
                ImGui::LoadIniSettingsFromDisk(iniPath.c_str());
            }
        };

        // Khởi tạo lần đầu với project mặc định
        updateIniPath(projectExplorer.getProjectPath());

        // --- Test: Create a simple entity ---
        std::string playerImagePath = projectExplorer.getCompletePath("assets/player.jpg");
        // ... (giữ nguyên phần tạo entity Player)
        auto player = engine.getWorld().entity("Player")
            .set<Levi::Position2D>({ 100.0f, 100.0f })
            .set<Levi::Sprite2D>({ playerImagePath, { 300.0f, 300.0f } });

        static bool resetLayout = false;

        engine.run([&]() {
            // --- 0. Setup Default Layout (Godot style) ---
            ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
            
            // Kiểm tra file ini tại vị trí mới
            bool iniExists = std::filesystem::exists(ImGui::GetIO().IniFilename);
            if ((engine.isFirstFrame() && !iniExists) || resetLayout) {
                // Xóa layout cũ và build layout mới (Dựa trên imgui.ini của bạn)
                ImGui::DockBuilderRemoveNode(dockspace_id); 
                ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
                ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);

                ImGuiID dock_main_id = dockspace_id;

                // 1. Chia bên trái (15% cho Hierarchy và Project Explorer)
                ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.15f, nullptr, &dock_main_id);

                // 2. Chia bên phải (25% cho Inspector)
                ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

                // 3. Chia dọc bên trái: Trên (Hierarchy 60%), Dưới (Project Explorer 40%)
                ImGuiID dock_id_left_top, dock_id_left_bottom;
                dock_id_left_top = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.60f, nullptr, &dock_id_left_bottom);

                // 4. Gán các cửa sổ vào node tương ứng
                ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_id_left_top);
                ImGui::DockBuilderDockWindow("Project Explorer", dock_id_left_bottom);
                ImGui::DockBuilderDockWindow("Inspector", dock_id_right);
                ImGui::DockBuilderDockWindow("Viewport", dock_main_id); // dock_main_id giờ là khu vực ở giữa (CentralNode)

                ImGui::DockBuilderFinish(dockspace_id);

                engine.clearFirstFrame();
                resetLayout = false;
            }


            // --- 1. Toolbar (Main Menu Bar) ---
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("Project")) {
                    if (ImGui::MenuItem("New Project")) {}
                    if (ImGui::MenuItem("Open Project...", "Ctrl+O")) {
                        nfdchar_t *outPath = NULL;
                        nfdresult_t result = NFD_PickFolder(&outPath, NULL);
                        if (result == NFD_OKAY) {
                            projectExplorer.setProjectPath(outPath);
                            updateIniPath(outPath); // Cập nhật file cấu hình cho project mới
                            NFD_FreePath(outPath);
                        }
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                        std::cout << "[Editor] Project Saved." << std::endl;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Editor")) {
                    if (ImGui::MenuItem("Save Layout")) {
                        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
                        std::cout << "[Editor] Layout Saved to: " << ImGui::GetIO().IniFilename << std::endl;
                    }
                    if (ImGui::MenuItem("Reset Layout")) {
                        resetLayout = true;
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            // --- 2. Viewport Window ---
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("Viewport");
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();
            SDL_Texture* viewportTex = engine.getViewportTexture();
            if (viewportTex) {
                ImGui::Image((ImTextureID)viewportTex, viewportSize);
            }
            ImGui::End();
            ImGui::PopStyleVar();

            // --- 3. Windows ---
            projectExplorer.render();
            sceneHierarchy.render(engine.getWorld());
            inspector.render(sceneHierarchy.getSelectedEntity());

            // ImGui::ShowDemoWindow();
        });
    }

    engine.shutdown();
    return 0;
}
