#pragma once

#include <string>
#include <filesystem>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace Levi {

    class ProjectExplorer {
    public:
        ProjectExplorer();
        ~ProjectExplorer();

        // Set the root directory of the project to display
        void setProjectPath(const std::string& path);
        const std::string& getProjectPath() const { return projectPath_; }
        std::string getCompletePath(const std::string& relativePath) const;

        // Main function to render the ImGui window
        void render();

    private:
        // Recursive function to draw the directory tree
        void drawDirectoryTree(const std::filesystem::path& dirPath);
        void openInSystem(const std::string& path);

        std::string projectPath_;
        std::filesystem::path selectedPath_;
    };

}
