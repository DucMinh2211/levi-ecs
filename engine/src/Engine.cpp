#include "Levi/Engine.h"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <iostream>

namespace Levi {
    EngineCore::EngineCore() : isRunning_(false), window_(nullptr), renderer_(nullptr), viewportTexture_(nullptr) {
        std::cout << "[Levi Engine] Constructed." << std::endl;
    }

    EngineCore::~EngineCore() {
        shutdown();
    }

    void EngineCore::createViewportTexture(int width, int height) {
        if (viewportTexture_) {
            SDL_DestroyTexture(viewportTexture_);
        }
        // Tạo texture có thể dùng làm đích đến để vẽ (TARGET)
        viewportTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
        if (!viewportTexture_) {
            std::cerr << "SDL_CreateTexture (Viewport) Error: " << SDL_GetError() << std::endl;
        }
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

        renderer_ = SDL_CreateRenderer(window_, nullptr); // nullptr to auto-select driver
        if (!renderer_) {
            std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
            return false;
        }

        // Create virtual screen for Viewport (Default 1920x1080 for good resolution)
        createViewportTexture(1920, 1080);

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

        setupSystems();

        assetManager_.init(renderer_);

        std::cout << "[Levi Engine] Initialized Successfully." << std::endl;
        isRunning_ = true;
        return true;
    }

    struct RendererRef {
        SDL_Renderer* ptr;
    };

    struct AssetManagerRef {
        AssetManager* ptr;
    };

    void EngineCore::setupSystems() {
        // Register Renderer and AssetManager pointers into ECS World as Singletons
        world_.set<RendererRef>({ renderer_ });
        world_.set<AssetManagerRef>({ &assetManager_ });

        // Create 2D Render System
        world_.system<const Position2D, const Sprite2D>()
            .kind(flecs::OnUpdate) // Runs every time world_.progress() is called
            .each([](flecs::entity e, const Position2D& pos, const Sprite2D& sprite) {
                // Access world from entity
                auto world = e.world();
                auto rRef = world.get<RendererRef>();
                auto amRef = world.get<AssetManagerRef>();

                if (!rRef || !rRef->ptr || !amRef || !amRef->ptr) return;

                // Try to load texture
                SDL_Texture* tex = amRef->ptr->loadTexture(sprite.texturePath);

                if (tex) {
                    // Draw actual texture
                    SDL_FRect dest = { pos.x, pos.y, sprite.size.x, sprite.size.y };
                    SDL_RenderTexture(rRef->ptr, tex, nullptr, &dest);
                } else {
                    // Fallback: Draw placeholder square (Yellow)
                    SDL_FRect rect = { pos.x, pos.y, sprite.size.x, sprite.size.y };
                    SDL_SetRenderDrawColor(rRef->ptr, 255, 255, 0, 255);
                    SDL_RenderFillRect(rRef->ptr, &rect);
                }
            });

        std::cout << "[Levi Engine] Systems Registered." << std::endl;
    }

    void EngineCore::run(std::function<void()> uiCallback) {
        if (!isRunning_) return;

        std::cout << "[Levi Engine] Running Main Loop..." << std::endl;
        
        SDL_Event event;
        while (isRunning_) {
            // 1. Process Events
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT) {
                    isRunning_ = false;
                }
            }

            // --- 2. Render to Viewport Texture (Game Logic) ---
            SDL_SetRenderTarget(renderer_, viewportTexture_); // Switch to virtual screen
            SDL_SetRenderDrawColor(renderer_, 20, 20, 20, 255); // Darker background color for the game
            SDL_RenderClear(renderer_);
            
            world_.progress(); // Run ECS Systems (Render System will draw into this Texture)
            
            SDL_SetRenderTarget(renderer_, nullptr); // Switch back to main screen
            // --------------------------------------------------

            // --- 3. Render Editor (Main Window) ---
            SDL_SetRenderDrawColor(renderer_, 33, 33, 33, 255);
            SDL_RenderClear(renderer_);
            
            beginFrame();

            if (uiCallback) {
                uiCallback(); // Editor will use ImGui::Image() to display viewportTexture_
            }

            endFrame();
            
            SDL_RenderPresent(renderer_);
        }
    }

    void EngineCore::beginFrame() {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport()); // Enable free window docking/dragging
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

            if (viewportTexture_) {
                SDL_DestroyTexture(viewportTexture_);
                viewportTexture_ = nullptr;
            }

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
