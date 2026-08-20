#pragma once

#include <string>
#include <filesystem>
#include <flecs.h>
#include <functional>
#include "Components.h"
#include "AssetManager.h"
#include "LuaScriptManager.h"

// Forward declarations to avoid including SDL3 in headers (reduces build time)
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace Levi {
    enum class PlayState { Edit, Playing, Paused };

    struct EngineConfig {
        std::string WindowTitle = "Levi Engine";
        int WindowWidth = 1280;
        int WindowHeight = 720;
    };

    class EngineCore {
    public:
        EngineCore();
        ~EngineCore();

        bool init(const EngineConfig& config);
        // callback function for ImGui
        void run(std::function<void()> uiCallback = nullptr);
        void shutdown();

        // ImGUI Helpers
        void beginFrame();
        void endFrame();

        // Project management
        bool loadProject(const std::string& projectPath);
        void unloadProject();
        bool loadScene(const std::filesystem::path& scenePath, std::string* error = nullptr);
        void setCurrentScenePath(const std::filesystem::path& scenePath);
        const std::filesystem::path& getCurrentScenePath() const { return currentScenePath_; }

        void play();
        void pause();
        void resume();
        void stop();
        PlayState getPlayState() const { return playState_; }

        // Getters
        flecs::world& getWorld() { return world_; }
        SDL_Renderer* getRenderer() { return renderer_; }
        SDL_Texture* getViewportTexture() { return viewportTexture_; }
        AssetManager& getAssetManager() { return assetManager_; }
        LuaScriptManager& getLuaScriptManager() { return luaScriptManager_; }
        bool isFirstFrame() const { return firstFrame_; }
        void clearFirstFrame() { firstFrame_ = false; }

    private:
        void setupSystems(); // Initializes ECS Systems
        void executeLuaSystems(); // Executes Lua-defined systems
        void processPendingSceneLoad();
        void createViewportTexture(int width, int height);

        bool isRunning_;
        bool firstFrame_ = true;
        PlayState playState_ = PlayState::Edit;
        std::string editSceneSnapshot_;
        std::filesystem::path projectPath_;
        std::filesystem::path currentScenePath_;
        std::filesystem::path editScenePathSnapshot_;
        SDL_Window* window_;
        SDL_Renderer* renderer_;
        SDL_Texture* viewportTexture_; // "Virtual screen" for Render to Texture
        
        // Members are destroyed in reverse order of declaration.
        // We want world_ to be destroyed LAST.
        flecs::world world_;
        AssetManager assetManager_;
        LuaScriptManager luaScriptManager_;
    };
}
