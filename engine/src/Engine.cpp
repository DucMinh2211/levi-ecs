#include "Levi/Engine.h"
#include <SDL3/SDL.h>
#include <iostream>

namespace Levi {
    EngineCore::EngineCore() : m_IsRunning(false), m_Window(nullptr), m_Renderer(nullptr) {
        std::cout << "[Levi Engine] Constructed." << std::endl;
    }

    EngineCore::~EngineCore() {
        Shutdown();
    }

    bool EngineCore::Init(const EngineConfig& config) {
        std::cout << "[Levi Engine] Initializing SDL3..." << std::endl;

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
            std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return false;
        }

        m_Window = SDL_CreateWindow(
            config.WindowTitle.c_str(),
            config.WindowWidth,
            config.WindowHeight,
            SDL_WINDOW_RESIZABLE
        );

        if (!m_Window) {
            std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            return false;
        }

        m_Renderer = SDL_CreateRenderer(m_Window, nullptr); // nullptr để tự chọn driver
        if (!m_Renderer) {
            std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            return false;
        }

        std::cout << "[Levi Engine] Initialized Successfully." << std::endl;
        m_IsRunning = true;
        return true;
    }

    void EngineCore::Run() {
        if (!m_IsRunning) return;

        std::cout << "[Levi Engine] Running Main Loop..." << std::endl;
        
        SDL_Event event;
        while (m_IsRunning) {
            // 1. Process Events
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    m_IsRunning = false;
                }
            }

            // 2. Update Logic (ECS)
            // TODO: flecs systems tick here

            // 3. Render
            SDL_SetRenderDrawColor(m_Renderer, 33, 33, 33, 255); // Màu xám đậm
            SDL_RenderClear(m_Renderer);
            
            // Vẽ các sprite ở đây
            
            SDL_RenderPresent(m_Renderer);
        }
    }

    void EngineCore::Shutdown() {
        if (m_Renderer) {
            SDL_DestroyRenderer(m_Renderer);
            m_Renderer = nullptr;
        }
        if (m_Window) {
            SDL_DestroyWindow(m_Window);
            m_Window = nullptr;
        }
        SDL_Quit();
        m_IsRunning = false;
        std::cout << "[Levi Engine] Shut down." << std::endl;
    }
}
