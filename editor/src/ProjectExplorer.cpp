#include "ProjectExplorer.h"
#include <filesystem>
#include <imgui.h>
#include <iostream>
#include <nfd.hpp>

namespace Levi {

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

    void ProjectExplorer::render() {
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
        // ... (phần còn lại của render giữ nguyên)
        if (std::filesystem::exists(projectPath_)) {
            drawDirectoryTree(projectPath_);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Project path does not exist!");
        }

        ImGui::End();
    }

    void ProjectExplorer::drawDirectoryTree(const std::filesystem::path& dirPath) {
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
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                    // Double click: Chỉ mở FILE bằng ứng dụng mặc định
                    if (!isDirectory) {
                        openInSystem(path.string());
                    }
                }

                // --- Menu Chuột Phải ---
                if (ImGui::BeginPopupContextItem()) {
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
                    drawDirectoryTree(path);
                    ImGui::TreePop();
                }
            }
        } catch (const std::exception& e) {}
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
