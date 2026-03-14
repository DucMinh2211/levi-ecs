#include "Levi/Engine.h"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <iostream>

namespace Levi {
    EngineCore::EngineCore() : isRunning_(false), window_(nullptr), renderer_(nullptr) {
        std::cout << "[Levi Engine] Constructed." << std::endl;
    }

    EngineCore::~EngineCore() {
        shutdown();
    }

    bool EngineCore::init(const EngineConfig& config) {
        std::cout << "[Levi Engine] Initializing SDL3..." << std::endl;

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
            std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return false;
        }

        window_ = SDL_CreateWindow(
            config.WindowTitle.c_str(),
            config.WindowWidth,
            config.WindowHeight,
            SDL_WINDOW_RESIZABLE
        );

        if (!window_) {
            std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            return false;
        }

        renderer_ = SDL_CreateRenderer(window_, nullptr); // nullptr để tự chọn driver
        if (!renderer_) {
            std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            return false;
        }

        // -- Initialize ImGUI --
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLRenderer(window_, renderer_);
        ImGui_ImplSDLRenderer3_Init(renderer_);

        std::cout << "[Levi Engine] Initialized Successfully." << std::endl;
        isRunning_ = true;
        return true;
    }

    void EngineCore::run(std::function<void()> uiCallback) {
        if (!isRunning_) return;

        std::cout << "[Levi Engine] Running Main Loop..." << std::endl;
        
        SDL_Event event;
        while (isRunning_) {
            // 1. Process Events
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event); // Send Event to ImGui
                if (event.type == SDL_EVENT_QUIT) {
                    isRunning_ = false;
                }
            }

            // 2. Update Logic (ECS)
            // TODONE: flecs systems tick here
            world_.progress();

            // 3. Render
            SDL_SetRenderDrawColor(renderer_, 33, 33, 33, 255); // Màu xám đậm
            SDL_RenderClear(renderer_);
            
            // -- Draw ImGui -- 
           beginFrame(); // Hàm này bạn cần hiện thực để gọi ImGui::NewFrame()

           if (uiCallback) {
               uiCallback(); // Gọi code giao diện của Editor
           }

           endFrame(); // Hàm này gọi ImGui::Render() và vẽ ra màn hình
           // -------------------------
            
            SDL_RenderPresent(renderer_);
        }
    }

    void EngineCore::beginFrame() {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void EngineCore::endFrame() {
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
    }

    void EngineCore::shutdown() {
        if (isRunning_) {
            ImGui_ImplSDLRenderer3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();

            if (renderer_) {
                SDL_DestroyRenderer(renderer_);
                renderer_ = nullptr;
            }
            if (window_) {
                SDL_DestroyWindow(window_);
                window_ = nullptr;
            }
            SDL_Quit();
            isRunning_ = false;
            std::cout << "[Levi Engine] Shut down." << std::endl;
        }
    }
}
