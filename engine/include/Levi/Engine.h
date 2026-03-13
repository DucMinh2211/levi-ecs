#pragma once

#include <string>

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

        bool Init(const EngineConfig& config);
        void Run();
        void Shutdown();

    private:
        bool m_IsRunning;
        SDL_Window* m_Window;
        SDL_Renderer* m_Renderer;
    };
}
