#include <imgui.h>
#include <imgui_internal.h>
#include <SDL3/SDL.h>
#include <iostream>
#include <filesystem>
#include <nfd.hpp>

#include "Levi/Engine.h"
#include "Levi/SceneSerializer.h"
#include "ProjectExplorer.h"
#include "SceneHierarchy.h"
#include "Inspector.h"
#include "SystemPanel.h"
#include "UndoRedoManager.h"

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
        Levi::SystemPanel systemPanel;
        Levi::UndoRedoManager undoRedo;

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
        
        // Load Lua scripts for project
        engine.loadProject(projectExplorer.getProjectPath());

        // Script files are loaded here so schemas and systems are available in
        // Edit mode. Runtime onInit() is deferred until the user presses Play.

        static bool resetLayout = false;
        std::filesystem::path activeScenePath;

        auto saveSceneToPath = [&](std::filesystem::path scenePath) {
            const std::string filename = scenePath.filename().string();
            const bool binary = filename.ends_with(".levscene.bin");
            if (!binary && !filename.ends_with(".levscene.json")) {
                if (scenePath.extension() == ".json") scenePath.replace_extension();
                scenePath += ".levscene.json";
            }

            std::string error;
            const bool saved = binary
                ? Levi::SceneSerializer::saveBinary(engine.getWorld(), scenePath, &error)
                : Levi::SceneSerializer::saveJson(engine.getWorld(), scenePath, &error);
            if (saved) {
                activeScenePath = scenePath;
                engine.setCurrentScenePath(scenePath);
                std::cout << "[Editor] Scene saved: " << scenePath << std::endl;
                return true;
            }
            std::cerr << "[Editor] Save failed: " << error << std::endl;
            return false;
        };

        auto saveSceneAsJson = [&]() {
            if (engine.getPlayState() != Levi::PlayState::Edit) {
                std::cerr << "[Editor] Stop Play mode before saving a scene." << std::endl;
                return false;
            }

            const auto scenesDir = std::filesystem::path(projectExplorer.getProjectPath()) / "scenes";
            std::filesystem::create_directories(scenesDir);
            const nfdfilteritem_t filters[] = {{"Levi JSON Scene", "levscene.json"}};
            nfdchar_t* outPath = nullptr;
            const nfdresult_t result = NFD_SaveDialog(
                &outPath, filters, 1, scenesDir.string().c_str(), "main.levscene.json");
            if (result == NFD_OKAY) {
                const std::filesystem::path chosenPath(outPath);
                NFD_FreePath(outPath);
                return saveSceneToPath(chosenPath);
            }
            if (result == NFD_ERROR) std::cerr << "[Editor] Save dialog failed: " << NFD_GetError() << std::endl;
            return false;
        };

        auto saveCurrentScene = [&]() {
            if (activeScenePath.empty()) return saveSceneAsJson();
            if (engine.getPlayState() != Levi::PlayState::Edit) {
                std::cerr << "[Editor] Stop Play mode before saving a scene." << std::endl;
                return false;
            }
            return saveSceneToPath(activeScenePath);
        };

        auto loadScenePath = [&](const std::filesystem::path& scenePath) {
            const std::string filename = scenePath.filename().string();
            const bool json = filename.ends_with(".levscene.json");
            const bool binary = filename.ends_with(".levscene.bin");
            if (!json && !binary) return false;
            if (engine.getPlayState() != Levi::PlayState::Edit) {
                std::cerr << "[Editor] Stop Play mode before loading a scene." << std::endl;
                return true;
            }

            std::string error;
            const bool loaded = engine.loadScene(scenePath, &error);
            if (!loaded) {
                std::cerr << "[Editor] Load failed: " << error << std::endl;
                return true;
            }

            activeScenePath = scenePath;
            undoRedo.clear();
            sceneHierarchy.setSelectedEntity(flecs::entity::null());
            std::cout << "[Editor] Scene loaded: " << scenePath << std::endl;
            return true;
        };

        auto showLoadSceneDialog = [&]() {
            if (engine.getPlayState() != Levi::PlayState::Edit) {
                std::cerr << "[Editor] Stop Play mode before loading a scene." << std::endl;
                return;
            }

            const auto scenesDir = std::filesystem::path(projectExplorer.getProjectPath()) / "scenes";
            std::filesystem::create_directories(scenesDir);
            const nfdfilteritem_t filters[] = {{"Levi Scene", "levscene.json,levscene.bin"}};
            nfdchar_t* outPath = nullptr;
            const nfdresult_t result = NFD_OpenDialog(&outPath, filters, 1, scenesDir.string().c_str());
            if (result == NFD_OKAY) {
                const std::filesystem::path chosenPath(outPath);
                NFD_FreePath(outPath);
                loadScenePath(chosenPath);
            } else if (result == NFD_ERROR) {
                std::cerr << "[Editor] Load dialog failed: " << NFD_GetError() << std::endl;
            }
        };

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

                // 2. Chia bên phải (25% cho Inspector và Systems)
                ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

                // 3. Chia dọc bên trái: Trên (Hierarchy 60%), Dưới (Project Explorer 40%)
                ImGuiID dock_id_left_top, dock_id_left_bottom;
                dock_id_left_top = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Up, 0.60f, nullptr, &dock_id_left_bottom);
                
                // 4. Chia dọc bên phải: Trên (Inspector 70%), Dưới (Systems 30%)
                ImGuiID dock_id_right_top, dock_id_right_bottom;
                dock_id_right_top = ImGui::DockBuilderSplitNode(dock_id_right, ImGuiDir_Up, 0.70f, nullptr, &dock_id_right_bottom);

                // 5. Gán các cửa sổ vào node tương ứng
                ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_id_left_top);
                ImGui::DockBuilderDockWindow("Project Explorer", dock_id_left_bottom);
                ImGui::DockBuilderDockWindow("Inspector", dock_id_right_top);
                ImGui::DockBuilderDockWindow("Systems", dock_id_right_bottom);
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
                            
                            // Reload Lua scripts for new project
                            undoRedo.clear();
                            engine.loadProject(outPath);
                            activeScenePath.clear();
                            sceneHierarchy.setSelectedEntity(flecs::entity::null());
                            
                            NFD_FreePath(outPath);
                        }
                    }
                    ImGui::Separator();
                    const bool canEditScene = engine.getPlayState() == Levi::PlayState::Edit;
                    if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, canEditScene)) saveCurrentScene();
                    if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S", false, canEditScene)) saveSceneAsJson();
                    if (ImGui::MenuItem("Load Scene...", "Ctrl+L", false, canEditScene)) showLoadSceneDialog();
                    if (ImGui::MenuItem("Export Scene (Binary)")) {
                        std::string error;
                        const auto scenePath = std::filesystem::path(projectExplorer.getProjectPath()) / "scenes" / "main.levscene.bin";
                        if (!Levi::SceneSerializer::saveBinary(engine.getWorld(), scenePath, &error)) {
                            std::cerr << "[Editor] Binary export failed: " << error << std::endl;
                        } else {
                            std::cout << "[Editor] Binary scene exported: " << scenePath << std::endl;
                        }
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Editor")) {
                    std::string undoLabel = undoRedo.canUndo()
                        ? std::string("Undo ") + undoRedo.undoName()
                        : "Undo";
                    if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, undoRedo.canUndo())) {
                        undoRedo.undo();
                    }

                    std::string redoLabel = undoRedo.canRedo()
                        ? std::string("Redo ") + undoRedo.redoName()
                        : "Redo";
                    if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, undoRedo.canRedo())) {
                        undoRedo.redo();
                    }
                    ImGui::Separator();
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

            ImGui::SetNextWindowPos(ImVec2(ImGui::GetMainViewport()->GetCenter().x - 110.0f, 24.0f), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.85f);
            ImGui::Begin("Play Controls", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking);
            const auto playState = engine.getPlayState();
            if (playState == Levi::PlayState::Edit) {
                if (ImGui::Button("Play")) { undoRedo.clear(); engine.play(); }
            } else {
                if (playState == Levi::PlayState::Playing) {
                    if (ImGui::Button("Pause")) engine.pause();
                } else if (ImGui::Button("Resume")) {
                    engine.resume();
                }
                ImGui::SameLine();
                if (ImGui::Button("Stop")) {
                    engine.stop();
                    undoRedo.clear();
                    sceneHierarchy.setSelectedEntity(flecs::entity::null());
                }
            }
            ImGui::End();

            ImGuiIO& io = ImGui::GetIO();
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
                if (io.KeyShift) saveSceneAsJson();
                else saveCurrentScene();
            }
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L, false)) showLoadSceneDialog();
            if (!ImGui::IsAnyItemActive()) {
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) undoRedo.undo();
                if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) undoRedo.redo();
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
            projectExplorer.render(engine.getAssetManager(), loadScenePath);
            sceneHierarchy.render(engine.getWorld(), undoRedo);
            inspector.render(sceneHierarchy.getSelectedEntity(), projectExplorer.getProjectPath(), undoRedo);
            systemPanel.render();

            // ImGui::ShowDemoWindow();
        });
    }

    engine.shutdown();
    return 0;
}
