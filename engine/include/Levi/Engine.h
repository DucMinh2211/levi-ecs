#pragma once

#include <string>
#include <flecs.h>
#include <functional>
#include "Components.h"
#include "AssetManager.h"

// Forward declarations to avoid including SDL3 in headers (reduces build time)
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace Levi {
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

        // Getters
        flecs::world& getWorld() { return world_; }
        SDL_Renderer* getRenderer() { return renderer_; }
        SDL_Texture* getViewportTexture() { return viewportTexture_; }
        AssetManager& getAssetManager() { return assetManager_; }
        bool isFirstFrame() const { return firstFrame_; }
        void clearFirstFrame() { firstFrame_ = false; }

    private:
        void setupSystems(); // Initializes ECS Systems
        void createViewportTexture(int width, int height);

        bool isRunning_;
        bool firstFrame_ = true;
        SDL_Window* window_;
        SDL_Renderer* renderer_;
        SDL_Texture* viewportTexture_; // "Virtual screen" for Render to Texture
        AssetManager assetManager_;
        flecs::world world_;
    };
}
