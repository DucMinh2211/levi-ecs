#include "Levi/Engine.h"
#include "Levi/SystemManager.h"
#include "Levi/ScriptComponent.h"
#include "Levi/Input.h"
#include "Levi/Modules.h"
#include "Levi/SceneSerializer.h"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <iostream>
#include <system_error>

namespace {
    bool reportSceneError(std::string* error, const std::string& message) {
        if (error) *error = message;
        return false;
    }
}

namespace Levi {
    EngineCore::EngineCore() : isRunning_(false), window_(nullptr), renderer_(nullptr), viewportTexture_(nullptr) {
        std::cout << "[Levi Engine] Constructed." << std::endl;
    }

    EngineCore::~EngineCore() {
        shutdown();
    }

    bool EngineCore::loadProject(const std::string& projectPath) {
        std::cout << "[Levi Engine] Loading project: " << projectPath << std::endl;

        std::error_code pathError;
        projectPath_ = std::filesystem::weakly_canonical(projectPath, pathError);
        if (pathError) projectPath_ = std::filesystem::path(projectPath).lexically_normal();
        currentScenePath_.clear();
        editScenePathSnapshot_.clear();

        assetManager_.setBasePath(projectPath_.string());

        // Initialize Lua scripts for this project
        if (!luaScriptManager_.init(projectPath_.string(), &world_)) {
            std::cerr << "[Levi Engine] Failed to initialize Lua for project: " << projectPath << std::endl;
            return false;
        }

        return true;
    }

    void EngineCore::unloadProject() {
        std::cout << "[Levi Engine] Unloading project..." << std::endl;
        luaScriptManager_.shutdown();
        currentScenePath_.clear();
        editScenePathSnapshot_.clear();
        projectPath_.clear();
    }

    void EngineCore::setCurrentScenePath(const std::filesystem::path& scenePath) {
        currentScenePath_ = scenePath.lexically_normal();

        std::error_code relativeError;
        const auto relative = std::filesystem::relative(currentScenePath_, projectPath_, relativeError);
        const bool outsideProject = relativeError || relative.empty()
            || (!relative.begin()->empty() && *relative.begin() == "..");
        luaScriptManager_.setCurrentScenePath(
            outsideProject ? currentScenePath_.generic_string() : relative.generic_string());
    }

    bool EngineCore::loadScene(const std::filesystem::path& scenePath, std::string* error) {
        const std::string filename = scenePath.filename().string();
        bool loaded = false;
        if (filename.ends_with(".levscene.json")) {
            loaded = SceneSerializer::loadJson(world_, scenePath, error);
        } else if (filename.ends_with(".levscene.bin")) {
            loaded = SceneSerializer::loadBinary(world_, scenePath, error);
        } else {
            return reportSceneError(error, "unsupported scene format: " + scenePath.string());
        }

        if (loaded) setCurrentScenePath(scenePath);
        return loaded;
    }

    void EngineCore::play() {
        if (playState_ == PlayState::Paused) {
            resume();
            return;
        }
        if (playState_ != PlayState::Edit) return;
        editSceneSnapshot_ = SceneSerializer::toJson(world_);
        editScenePathSnapshot_ = currentScenePath_;
        playState_ = PlayState::Playing;
        luaScriptManager_.startRuntime();
    }

    void EngineCore::pause() {
        if (playState_ == PlayState::Playing) playState_ = PlayState::Paused;
    }

    void EngineCore::resume() {
        if (playState_ == PlayState::Paused) playState_ = PlayState::Playing;
    }

    void EngineCore::stop() {
        if (playState_ == PlayState::Edit) return;
        luaScriptManager_.stopRuntime();
        std::string error;
        if (!editSceneSnapshot_.empty() && !SceneSerializer::fromJson(world_, editSceneSnapshot_, &error)) {
            std::cerr << "[Editor] Failed to restore edit scene: " << error << std::endl;
        }
        currentScenePath_ = editScenePathSnapshot_;
        if (!currentScenePath_.empty()) setCurrentScenePath(currentScenePath_);
        else luaScriptManager_.setCurrentScenePath({});
        editSceneSnapshot_.clear();
        editScenePathSnapshot_.clear();
        playState_ = PlayState::Edit;
    }

    void EngineCore::processPendingSceneLoad() {
        auto request = luaScriptManager_.takePendingSceneLoad();
        if (!request || request->empty() || playState_ == PlayState::Edit) return;

        std::filesystem::path requestedPath(*request);
        if (!requestedPath.is_absolute()) requestedPath = projectPath_ / requestedPath;

        std::error_code pathError;
        const auto resolved = std::filesystem::weakly_canonical(requestedPath, pathError);
        if (pathError) {
            const std::string message = "cannot resolve scene path: " + requestedPath.string();
            std::cerr << "[Scene] " << message << std::endl;
            luaScriptManager_.callFunction("onSceneLoadFailed", *request, message);
            return;
        }

        const auto relative = resolved.lexically_relative(projectPath_);
        if (relative.empty() || (!relative.begin()->empty() && *relative.begin() == "..")) {
            const std::string message = "runtime scene must be inside the active project: " + resolved.string();
            std::cerr << "[Scene] " << message << std::endl;
            luaScriptManager_.callFunction("onSceneLoadFailed", *request, message);
            return;
        }

        const std::string filename = resolved.filename().string();
        if ((!filename.ends_with(".levscene.json") && !filename.ends_with(".levscene.bin"))
            || !std::filesystem::is_regular_file(resolved)) {
            const std::string message = "runtime scene does not exist or has an unsupported format: "
                + resolved.string();
            std::cerr << "[Scene] " << message << std::endl;
            luaScriptManager_.callFunction("onSceneLoadFailed", *request, message);
            return;
        }

        const std::string previousScene = luaScriptManager_.getCurrentScenePath();
        luaScriptManager_.callFunction("onSceneUnload", previousScene);

        std::string error;
        if (!loadScene(resolved, &error)) {
            std::cerr << "[Scene] Runtime load failed: " << error << std::endl;
            luaScriptManager_.callFunction("onSceneLoadFailed", *request, error);
            return;
        }

        // The serializer has replaced every scene entity. Old per-script IDs
        // must not survive and accidentally target a future recycled Flecs ID.
        luaScriptManager_.forgetCreatedEntities();
        std::cout << "[Scene] Runtime scene loaded: " << resolved << std::endl;
        luaScriptManager_.callFunction("onSceneLoaded", luaScriptManager_.getCurrentScenePath());
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

        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
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

    void EngineCore::setupSystems() {
        // Register Renderer and AssetManager pointers into ECS World as Singletons
        world_.set<RendererRef>({ renderer_ });
        world_.set<AssetManagerRef>({ &assetManager_ });

        world_.import<TransformModule>();
        world_.import<RenderModule>();

        std::cout << "[Levi Engine] Systems Registered." << std::endl;
    }

    void EngineCore::executeLuaSystems() {
        auto& systemManager = SystemManager::getInstance();
        for (auto& sys : systemManager.getSystems()) {
            if (!sys.isEnabled) continue;

            auto builder = world_.query_builder();
            for (const auto& compName : sys.queryComponents) {
                if (compName == "Position2D") builder.with<Position2D>();
                else if (compName == "Scale2D") builder.with<Scale2D>();
                else if (compName == "Rotation2D") builder.with<Rotation2D>();
                else if (compName == "Sprite2D") builder.with<Sprite2D>();
                else if (compName == "AABBCollider2D") builder.with<AABBCollider2D>();
                else if (compName == "CircleCollider2D") builder.with<CircleCollider2D>();
                else if (compName == "Camera2D") builder.with<Camera2D>();
                else {
                    builder.with<ScriptComponent>(world_.entity(compName.c_str()));
                }
            }
            
            auto f = builder.build();
            f.each([&sys](flecs::entity e) {
                auto result = sys.callback(e.id());
                if (!result.valid()) {
                    sol::error err = result;
                    std::cerr << "[Lua System Error] " << sys.name << ": " << err.what() << std::endl;
                }
            });
        }
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
            Input::instance().update();

            // --- 2. Render to Viewport Texture (Game Logic) ---
            SDL_SetRenderTarget(renderer_, viewportTexture_); // Switch to virtual screen
            SDL_SetRenderDrawColor(renderer_, 20, 20, 20, 255); // Darker background color for the game
            SDL_RenderClear(renderer_);
            
            // Check for Lua script changes (hot reload)
            luaScriptManager_.checkForChanges();
            
            if (playState_ == PlayState::Playing) {
                world_.defer_begin();
                luaScriptManager_.callFunction("onUpdate", world_.delta_time());
                if (!luaScriptManager_.hasPendingSceneLoad()) executeLuaSystems();
                world_.defer_end();
            }

            if (luaScriptManager_.isRuntimeActive() && luaScriptManager_.hasPendingSceneLoad()) {
                processPendingSceneLoad();
            }
            
            world_.set_time_scale(playState_ == PlayState::Playing ? 1.0f : 0.0f);
            world_.progress(); // Rendering continues while simulation receives a zero delta in Edit/Pause.
            
            SDL_SetRenderTarget(renderer_, nullptr); // Switch back to main screen
            // --------------------------------------------------

            // --- 3. Render Editor (Main Window) ---
            SDL_SetRenderDrawColor(renderer_, 33, 33, 33, 255);
            SDL_RenderClear(renderer_);
            
            beginFrame();

            if (uiCallback) {
                world_.defer_begin();
                uiCallback(); // Editor will use ImGui::Image() to display viewportTexture_
                world_.defer_end();
            }

            endFrame();
            
            SDL_RenderPresent(renderer_);
        }
    }

    void EngineCore::beginFrame() {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        
        // Đặt tên cố định cho DockSpace để lưu layout chính xác vào .ini
        ImGui::DockSpaceOverViewport(ImGui::GetID("MainDockSpace"), ImGui::GetMainViewport());
    }

    void EngineCore::endFrame() {
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_);
    }

    void EngineCore::shutdown() {
        if (isRunning_) {
            // Unload any active project
            unloadProject();

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
