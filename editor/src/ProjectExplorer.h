#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "Levi/AssetManager.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace Levi {

    class ProjectExplorer {
    public:
        using FileOpenHandler = std::function<bool(const std::filesystem::path&)>;

        ProjectExplorer();
        ~ProjectExplorer();

        // Set the root directory of the project to display
        void setProjectPath(const std::string& path);
        const std::string& getProjectPath() const { return projectPath_; }
        std::string getCompletePath(const std::string& relativePath) const;

        // Main function to render the ImGui window
        void render(AssetManager& assets, const FileOpenHandler& onOpenFile = {});

    private:
        // Recursive function to draw the directory tree
        void drawDirectoryTree(const std::filesystem::path& dirPath, const FileOpenHandler& onOpenFile);
        void drawAssetGrid(AssetManager& assets, const FileOpenHandler& onOpenFile);
        void activateFile(const std::filesystem::path& path, const FileOpenHandler& onOpenFile);
        void openInSystem(const std::string& path);

        std::string projectPath_;
        std::filesystem::path selectedPath_;
    };

} // namespace Levi
