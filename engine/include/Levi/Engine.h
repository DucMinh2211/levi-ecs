#pragma once

#include <string>
#include <flecs.h>
#include <functional>

// Forward declarations để tránh include SDL3 vào header (giảm thời gian build)
struct SDL_Window;
struct SDL_Renderer;

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

    private:
        bool isRunning_;
        SDL_Window* window_;
        SDL_Renderer* renderer_;
        flecs::world world_;
    };
}
