#include "ProjectExplorer.h"
#include <filesystem>
#include <imgui.h>
#include <iostream>

namespace Levi {

    ProjectExplorer::ProjectExplorer() {
        // Default to the current demo-project (./projects/demo-project/)
        std::filesystem::path currentPath = std::filesystem::current_path();
        
        // If we are running from 'bin' or 'build' (or any folder where 'projects' doesn't exist), 
        // try to go up one level to find the repository root.
        if (!std::filesystem::exists(currentPath / "projects") && currentPath.has_parent_path()) {
            std::filesystem::path parentPath = currentPath.parent_path();
            if (std::filesystem::exists(parentPath / "projects")) {
                currentPath = parentPath;
            }
        }

        std::filesystem::path projectPath = currentPath / "projects" / "demo-project";
        projectPath_ = projectPath.make_preferred().string();
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

        // Display current path and a way to change it (simple text input for now)
        char pathBuf[512];
        strncpy(pathBuf, projectPath_.c_str(), sizeof(pathBuf));
        if (ImGui::InputText("Project Root", pathBuf, sizeof(pathBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
            setProjectPath(pathBuf);
        }

        ImGui::Separator();

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

                // Skip hidden files/folders (starting with '.')
                if (filename[0] == '.') continue;

                if (entry.is_directory()) {
                    // It's a directory, create a tree node
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
                    if (selectedPath_ == path) flags |= ImGuiTreeNodeFlags_Selected;

                    bool nodeOpen = ImGui::TreeNodeEx(filename.c_str(), flags);
                    
                    if (ImGui::IsItemClicked()) {
                        selectedPath_ = path;
                    }

                    if (nodeOpen) {
                        drawDirectoryTree(path);
                        ImGui::TreePop();
                    }
                } else {
                    // It's a file, just a selectable text
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
                    if (selectedPath_ == path) flags |= ImGuiTreeNodeFlags_Selected;

                    ImGui::TreeNodeEx(filename.c_str(), flags);
                    
                    if (ImGui::IsItemClicked()) {
                        selectedPath_ = path;
                    }
                }
            }
        } catch (const std::exception& e) {
            // Silently ignore or show error
        }
    }

}
