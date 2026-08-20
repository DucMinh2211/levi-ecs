#include "ProjectExplorer.h"
#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <nfd.hpp>
#include <SDL3/SDL.h>

namespace Levi {

    namespace {
        bool isSceneFile(const std::filesystem::path& path) {
            const std::string filename = path.filename().string();
            return filename.ends_with(".levscene.json") || filename.ends_with(".levscene.bin");
        }
    }

    ProjectExplorer::ProjectExplorer() {
        // Khởi tạo NFD (Quan trọng để chạy được trên các OS)
        NFD_Init();

        // Default to the current demo-project (./projects/demo-project/)
        std::filesystem::path currentPath = std::filesystem::current_path();
        if (!std::filesystem::exists(currentPath / "projects") && currentPath.has_parent_path()) {
            std::filesystem::path parentPath = currentPath.parent_path();
            if (std::filesystem::exists(parentPath / "projects")) {
                currentPath = parentPath;
            }
        }
        std::filesystem::path projectPath = currentPath / "projects" / "demo-project";
        projectPath_ = projectPath.make_preferred().string();
    }

    ProjectExplorer::~ProjectExplorer() {
        NFD_Quit(); // Giải phóng NFD khi đóng Project Explorer
    }

    void ProjectExplorer::setProjectPath(const std::string& path) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            projectPath_ = path;
            std::cout << "[ProjectExplorer] Set project root to: " << projectPath_ << std::endl;
        } else {
            std::cerr << "[ProjectExplorer] Invalid project path: " << path << std::endl;
        }
    }

    std::string ProjectExplorer::getCompletePath(const std::string& relativePath) const {
        return (std::filesystem::path(projectPath_) / relativePath).make_preferred().string();
    }

    void ProjectExplorer::render(AssetManager& assets, const FileOpenHandler& onOpenFile) {
        ImGui::Begin("Project Explorer");

        // Display current path
        char pathBuf[512];
        strncpy(pathBuf, projectPath_.c_str(), sizeof(pathBuf));
        
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
        if (ImGui::InputText("##ProjectRoot", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            setProjectPath(pathBuf);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("...")) {
            nfdchar_t *outPath = NULL;
            nfdresult_t result = NFD_PickFolder(&outPath, NULL);

            if (result == NFD_OKAY) {
                setProjectPath(outPath);
                NFD_FreePath(outPath);
            } else if (result == NFD_CANCEL) {
                // User cancelled, do nothing
            } else {
                std::cerr << "NFD Error: " << NFD_GetError() << std::endl;
            }
        }

        ImGui::Separator();
        if (std::filesystem::exists(projectPath_)) {
            ImGui::BeginChild("AssetTree", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.45f), true);
            drawDirectoryTree(projectPath_, onOpenFile);
            ImGui::EndChild();
            drawAssetGrid(assets, onOpenFile);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Project path does not exist!");
        }

        ImGui::End();
    }

    void ProjectExplorer::drawAssetGrid(AssetManager& assets, const FileOpenHandler& onOpenFile) {
        std::filesystem::path directory = selectedPath_;
        if (directory.empty() || !std::filesystem::is_directory(directory)) directory = directory.parent_path();
        if (directory.empty() || !std::filesystem::exists(directory)) directory = projectPath_;

        ImGui::SeparatorText(directory.filename().string().c_str());
        ImGui::BeginChild("AssetGrid", ImVec2(0, 0), true);
        const float cellSize = 92.0f;
        const int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cellSize));
        ImGui::Columns(columns, nullptr, false);
        try {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (!entry.is_regular_file()) continue;
                const auto extension = entry.path().extension().string();
                const bool isImage = extension == ".png" || extension == ".jpg" || extension == ".jpeg"
                    || extension == ".bmp" || extension == ".gif";
                const std::string relative = std::filesystem::relative(entry.path(), projectPath_).generic_string();
                ImGui::PushID(relative.c_str());
                if (isImage) {
                    SDL_Texture* texture = assets.loadTexture(relative);
                    if (texture) ImGui::Image(reinterpret_cast<ImTextureID>(texture), ImVec2(64, 64));
                    else ImGui::Button("Image", ImVec2(64, 64));
                } else {
                    ImGui::Button("File", ImVec2(64, 64));
                }
                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("LEVI_ASSET_PATH", relative.c_str(), relative.size() + 1);
                    ImGui::TextUnformatted(relative.c_str());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) activateFile(entry.path(), onOpenFile);
                ImGui::TextWrapped("%s", entry.path().filename().string().c_str());
                ImGui::NextColumn();
                ImGui::PopID();
            }
        } catch (const std::exception&) {}
        ImGui::Columns(1);
        ImGui::EndChild();
    }

    void ProjectExplorer::drawDirectoryTree(const std::filesystem::path& dirPath, const FileOpenHandler& onOpenFile) {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                const auto& path = entry.path();
                const auto& filename = path.filename().string();

                if (filename[0] == '.') continue;

                bool isDirectory = entry.is_directory();
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selectedPath_ == path) flags |= ImGuiTreeNodeFlags_Selected;
                if (!isDirectory) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

                bool nodeOpen = ImGui::TreeNodeEx(filename.c_str(), flags);
                
                // --- Xử lý Click & Double Click ---
                if (ImGui::IsItemClicked()) {
                    selectedPath_ = path;
                    if (!isDirectory && isSceneFile(path) && onOpenFile) onOpenFile(path);
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    // Scene files are loaded on the first click above. Other
                    // file types keep the usual double-click open behavior.
                    if (!isDirectory && !isSceneFile(path)) activateFile(path, onOpenFile);
                }

                // --- Menu Chuột Phải ---
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Copy as path")) {
                        ImGui::SetClipboardText(path.string().c_str());
                    }
                    if (ImGui::MenuItem("Copy relative path")) {
                        std::filesystem::path relPath = std::filesystem::relative(path, projectPath_);
                        ImGui::SetClipboardText(relPath.string().c_str());
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Open in Explorer/Finder")) {
                        // Nếu là file, ta nên mở folder chứa nó (hoặc tùy OS xử lý)
                        openInSystem(isDirectory ? path.string() : path.parent_path().string());
                    }
                    if (!isDirectory && ImGui::MenuItem("Open with Default App")) {
                        openInSystem(path.string());
                    }
                    ImGui::EndPopup();
                }

                if (nodeOpen && isDirectory) {
                    drawDirectoryTree(path, onOpenFile);
                    ImGui::TreePop();
                }
            }
        } catch (const std::exception& e) {}
    }

    void ProjectExplorer::activateFile(const std::filesystem::path& path, const FileOpenHandler& onOpenFile) {
        if (onOpenFile && onOpenFile(path)) return;
        openInSystem(path.string());
    }

    void ProjectExplorer::openInSystem(const std::string& path) {
#ifdef _WIN32
        ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#elif __APPLE__
        std::string command = "open \"" + path + "\"";
        system(command.c_str());
#else
        std::string command = "xdg-open \"" + path + "\"";
        system(command.c_str());
#endif
    }

}
