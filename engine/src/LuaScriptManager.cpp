#include "Levi/LuaScriptManager.h"
#include "Levi/Components.h"
#include "Levi/Math.h"
#include "Levi/SystemManager.h"
#include "Levi/Input.h"
#include "Levi/Physics2D.h"
#include "Levi/Camera2D.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

namespace {

    void writeTextFileIfChanged(const std::filesystem::path& path, const std::string& content) {
        std::ifstream currentFile(path, std::ios::binary);
        if (currentFile) {
            std::ostringstream current;
            current << currentFile.rdbuf();
            if (current.str() == content) return;
        }

        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("Cannot write " + path.string());
        output << content;
        if (!output) throw std::runtime_error("Failed while writing " + path.string());
    }

}

namespace Levi {

    LuaScriptManager::LuaScriptManager() : initialized_(false), world_(nullptr) {
        std::cout << "[LuaScriptManager] Constructed." << std::endl;
    }

    LuaScriptManager::~LuaScriptManager() {
        shutdown();
    }

    bool LuaScriptManager::init(const std::string& projectPath, flecs::world* world) {
        if (initialized_) {
            shutdown();
        }

        projectPath_ = std::filesystem::path(projectPath).lexically_normal().string();
        scriptsFolder_ = (std::filesystem::path(projectPath_) / "scripts").string();
        world_ = world;

        if (!world_) return false;

        try {
            lua_ = sol::state();
            lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table);

            // Resolve modules relative to both the project root and scripts folder.
            std::string path = lua_["package"]["path"];
            const std::string projectLuaPath = std::filesystem::path(projectPath_).generic_string();
            const std::string scriptsLuaPath = std::filesystem::path(scriptsFolder_).generic_string();
            path += ";" + projectLuaPath + "/?.lua";
            path += ";" + projectLuaPath + "/?/init.lua";
            path += ";" + scriptsLuaPath + "/?.lua";
            path += ";" + scriptsLuaPath + "/?/init.lua";
            lua_["package"]["path"] = path;

            bindECS(world_);
            exportAPIDefinitions();
            
            initialized_ = true;
            std::cout << "[LuaScriptManager] Lua VM initialized for project: " << projectPath << std::endl;

            return loadAllScripts();
        } catch (const std::exception& e) {
            std::cerr << "[LuaScriptManager] Init Error: " << e.what() << std::endl;
            return false;
        }
    }

    void LuaScriptManager::bindECS(flecs::world* world) {
        if (!world) return;
        
        // Register enums
        lua_.new_enum("PivotType", 
            "Percent", PivotType::Percent,
            "Pixel", PivotType::Pixel
        );

        // Register types for Lua
        lua_.new_usertype<Position2D>("Position2D", "x", &Position2D::x, "y", &Position2D::y);
        lua_.new_usertype<Scale2D>("Scale2D", "x", &Scale2D::x, "y", &Scale2D::y);
        lua_.new_usertype<Rotation2D>("Rotation2D", 
            "angle", &Rotation2D::angle,
            "pivot", &Rotation2D::pivot,
            "pivotType", &Rotation2D::pivotType
        );
        lua_.new_usertype<Vector2>("Vector2", "x", &Vector2::x, "y", &Vector2::y);
        lua_.new_usertype<Sprite2D>("Sprite2D", "texturePath", &Sprite2D::texturePath, "size", &Sprite2D::size);
        lua_.new_usertype<AABBCollider2D>("AABBCollider2D", "size", &AABBCollider2D::size, "offset", &AABBCollider2D::offset);
        lua_.new_usertype<CircleCollider2D>("CircleCollider2D", "radius", &CircleCollider2D::radius, "offset", &CircleCollider2D::offset);
        lua_.new_usertype<Camera2D>("Camera2D",
            "zoom", &Camera2D::zoom,
            "active", &Camera2D::active,
            "maxShakeOffset", &Camera2D::maxShakeOffset,
            "maxShakeRotation", &Camera2D::maxShakeRotation);

        auto ecs = lua_.create_table();

        // --- CORE API ---
        
        ecs["createEntity"] = [world](sol::optional<std::string> name) -> uint64_t {
            if (name && !name->empty()) return world->entity(name->c_str()).id();
            return world->entity().id();
        };

        ecs["deleteEntity"] = [world](uint64_t id) {
            auto e = world->entity(id);
            if (e.is_alive()) e.destruct();
        };

        // --- Component Management ---

        // Position
        ecs["addPosition"] = [world](uint64_t id, float x, float y) {
            world->entity(id).set<Position2D>({x, y});
        };
        ecs["setPosition"] = [world](uint64_t id, float x, float y) {
            world->entity(id).set<Position2D>({x, y});
        };
        ecs["getPosition"] = [world](uint64_t id) -> sol::optional<Position2D> {
            auto e = world->entity(id);
            if (e.is_alive() && e.has<Position2D>()) return *e.get<Position2D>();
            return sol::nullopt;
        };

        // Scale
        ecs["addScale"] = [world](uint64_t id, float x, float y) {
            world->entity(id).set<Scale2D>({x, y});
        };
        ecs["setScale"] = [world](uint64_t id, float x, float y) {
            world->entity(id).set<Scale2D>({x, y});
        };
        ecs["getScale"] = [world](uint64_t id) -> sol::optional<Scale2D> {
            auto e = world->entity(id);
            if (e.is_alive() && e.has<Scale2D>()) return *e.get<Scale2D>();
            return sol::nullopt;
        };

        // Rotation
        ecs["addRotation"] = [world](uint64_t id, float angle) {
            world->entity(id).set<Rotation2D>({angle});
        };
        ecs["setRotation"] = [world](uint64_t id, float angle) {
            auto e = world->entity(id);
            if (e.is_alive()) {
                if (auto rot = e.get_mut<Rotation2D>()) {
                    rot->angle = angle;
                } else {
                    e.set<Rotation2D>({angle});
                }
            }
        };
        ecs["setRotationPivot"] = [world](uint64_t id, float x, float y, PivotType type) {
            auto e = world->entity(id);
            if (e.is_alive()) {
                if (auto rot = e.get_mut<Rotation2D>()) {
                    rot->pivot = {x, y};
                    rot->pivotType = type;
                }
            }
        };
        ecs["getRotation"] = [world](uint64_t id) -> sol::optional<Rotation2D> {
            auto e = world->entity(id);
            if (e.is_alive() && e.has<Rotation2D>()) return *e.get<Rotation2D>();
            return sol::nullopt;
        };

        // Sprite
        ecs["addSprite"] = [this, world](uint64_t id, const std::string& path, float w, float h) {
            std::string fullPath = (std::filesystem::path(this->projectPath_) / path).string();
            world->entity(id).set<Sprite2D>({fullPath, {w, h}});
        };
        ecs["getSprite"] = [world](uint64_t id) -> sol::optional<Sprite2D> {
            auto e = world->entity(id);
            if (e.is_alive() && e.has<Sprite2D>()) return *e.get<Sprite2D>();
            return sol::nullopt;
        };

        ecs["addAABBCollider"] = [world](uint64_t id, float width, float height) {
            auto entity = world->entity(id);
            if (entity.is_alive()) entity.set<AABBCollider2D>({{width, height}, {0.0f, 0.0f}});
        };
        ecs["addCircleCollider"] = [world](uint64_t id, float radius) {
            auto entity = world->entity(id);
            if (entity.is_alive()) entity.set<CircleCollider2D>({radius, {0.0f, 0.0f}});
        };
        ecs["addCamera"] = [world](uint64_t id, sol::optional<float> zoom) {
            auto entity = world->entity(id);
            if (!entity.is_alive()) return;
            if (!entity.has<Position2D>()) entity.set<Position2D>({0.0f, 0.0f});
            Camera2D camera;
            camera.zoom = Camera2DSystem::clampZoom(zoom.value_or(1.0f));
            entity.set<Camera2D>(camera);
        };
        ecs["getCamera"] = [world](uint64_t id) -> sol::optional<Camera2D> {
            auto entity = world->entity(id);
            if (entity.is_alive() && entity.has<Camera2D>()) return *entity.get<Camera2D>();
            return sol::nullopt;
        };

        // --- Dynamic Script Components (ECS Abstraction Phase 1) ---

        ecs["AssetPath"] = [this](std::string path) -> sol::table {
            sol::table t = lua_.create_table();
            t["__type"] = "AssetPath";
            t["value"] = path;
            return t;
        };

        ecs["defineComponent"] = [world](std::string name, sol::table defaultValues) {
            ScriptComponentSchema schema;
            schema.name = name;
            
            for (auto& pair : defaultValues) {
                std::string fieldName = pair.first.as<std::string>();
                ScriptFieldType type = ScriptFieldType::Unknown;
                
                sol::object val = pair.second;
                if (val.is<float>() || val.is<double>()) type = ScriptFieldType::Float;
                else if (val.is<int>()) type = ScriptFieldType::Int;
                else if (val.is<std::string>()) type = ScriptFieldType::String;
                else if (val.is<bool>()) type = ScriptFieldType::Bool;
                else if (val.is<sol::table>()) {
                    sol::table t = val.as<sol::table>();
                    if (t["__type"] == "AssetPath") {
                        type = ScriptFieldType::AssetPath;
                    }
                }
                
                schema.fields.push_back({fieldName, type});
            }
            
            ScriptComponentRegistry::getInstance().registerSchema(schema);
            world->entity(name.c_str()).add<ScriptComponentSchemaTag>();
            std::cout << "[Lua] Defined Component: " << name << " with " << schema.fields.size() << " fields." << std::endl;
        };

        ecs["addComponent"] = [world](uint64_t id, std::string schemaName) {
            auto e = world->entity(id);
            if (!e.is_alive()) return;
            
            const auto* schema = ScriptComponentRegistry::getInstance().getSchema(schemaName);
            if (!schema) {
                std::cerr << "[Lua] Error: Component schema not found: " << schemaName << std::endl;
                return;
            }
            
            ScriptComponent comp;
            comp.schemaName = schemaName;

            // Initialize default values from schema
            for (const auto& field : schema->fields) {
                // For AssetPath, we might want to extract the default path if we stored it
                // Currently defineComponent doesn't store default values in the schema, 
                // but ScriptComponent initialization in setComponentValue or Inspector handles it.
            }
            
            // Attach as a pair (ScriptComponent, schemaName) to allow multiple script components on one entity
            e.set<ScriptComponent>(world->entity(schemaName.c_str()), comp);
        };

        ecs["setComponentValue"] = [world](uint64_t id, std::string schemaName, std::string fieldName, sol::object value) {
            auto e = world->entity(id);
            if (!e.is_alive()) return;

            auto* comp = e.get_mut<ScriptComponent>(world->entity(schemaName.c_str()));
            if (!comp) return;

            if (value.is<float>() || value.is<double>()) comp->values[fieldName] = (float)value.as<float>();
            else if (value.is<int>()) comp->values[fieldName] = value.as<int>();
            else if (value.is<std::string>()) comp->values[fieldName] = value.as<std::string>();
            else if (value.is<bool>()) comp->values[fieldName] = value.as<bool>();
            else if (value.is<sol::table>()) {
                sol::table t = value.as<sol::table>();
                if (t["__type"] == "AssetPath") {
                    comp->values[fieldName] = t.get<std::string>("value");
                }
            }
        };

        ecs["getComponentValue"] = [world](uint64_t id, std::string schemaName, std::string fieldName, sol::this_state L) -> sol::object {
            auto e = world->entity(id);
            if (!e.is_alive()) return sol::nil;

            const auto* comp = e.get<ScriptComponent>(world->entity(schemaName.c_str()));
            if (!comp) return sol::nil;

            auto it = comp->values.find(fieldName);
            if (it == comp->values.end()) return sol::nil;

            return std::visit([L](auto&& arg) -> sol::object {
                return sol::make_object(L, arg);
            }, it->second);
        };

        // --- System Management (ECS Abstraction Phase 2) ---

        ecs["registerSystem"] = [](std::string name, sol::table query, sol::protected_function callback) {
            std::vector<std::string> queryComponents;
            for (auto& pair : query) {
                queryComponents.push_back(pair.second.as<std::string>());
            }
            SystemManager::getInstance().registerSystem(name, queryComponents, callback);
            std::cout << "[Lua] Registered System: " << name << std::endl;
        };

        lua_["ECS"] = ecs;

        auto input = lua_.create_table();
        input["isKeyDown"] = [](const std::string& key) { return Input::instance().isKeyDown(key); };
        input["isKeyPressed"] = [](const std::string& key) { return Input::instance().isKeyPressed(key); };
        input["isKeyReleased"] = [](const std::string& key) { return Input::instance().isKeyReleased(key); };
        input["isMouseButtonDown"] = [](int button) { return Input::instance().isMouseButtonDown(button); };
        input["isMouseButtonPressed"] = [](int button) { return Input::instance().isMouseButtonPressed(button); };
        input["getMousePosition"] = [this]() {
            sol::table result = lua_.create_table();
            result["x"] = Input::instance().mouseX();
            result["y"] = Input::instance().mouseY();
            return result;
        };
        lua_["Input"] = input;

        auto physics = lua_.create_table();
        physics["overlaps"] = [world](uint64_t first, uint64_t second) {
            return Physics2D::overlaps(world->entity(first), world->entity(second));
        };
        physics["overlapsAABB"] = &Physics2D::overlapsAABB;
        physics["overlapsCircle"] = &Physics2D::overlapsCircle;
        lua_["Physics"] = physics;

        auto cameraApi = lua_.create_table();
        cameraApi["getActive"] = [world]() -> sol::optional<uint64_t> {
            auto entity = Camera2DSystem::findActive(*world);
            return entity ? sol::optional<uint64_t>(entity.id()) : sol::nullopt;
        };
        cameraApi["setActive"] = [world](uint64_t id) {
            auto entity = world->entity(id);
            if (!entity.is_alive() || !entity.has<Camera2D>() || !entity.has<Position2D>()) return false;
            Camera2DSystem::setActive(*world, id);
            return true;
        };
        cameraApi["move"] = [world](float x, float y) { return Camera2DSystem::move(*world, x, y); };
        cameraApi["setPosition"] = [world](float x, float y) { return Camera2DSystem::setPosition(*world, x, y); };
        cameraApi["setZoom"] = [world](float zoom) { return Camera2DSystem::setZoom(*world, zoom); };
        cameraApi["getZoom"] = [world]() { return Camera2DSystem::getZoom(*world); };
        cameraApi["shake"] = [world](float intensity, float duration) {
            return Camera2DSystem::shake(*world, intensity, duration);
        };
        lua_["Camera"] = cameraApi;

        auto scene = lua_.create_table();
        scene["load"] = [this](const std::string& path) {
            if (!runtimeActive_ || path.empty()) return false;
            pendingScenePath_ = path;
            return true;
        };
        scene["reload"] = [this]() {
            if (!runtimeActive_ || currentScenePath_.empty()) return false;
            pendingScenePath_ = currentScenePath_;
            return true;
        };
        scene["current"] = [this]() { return currentScenePath_; };
        lua_["Scene"] = scene;
    }

    void LuaScriptManager::shutdown() {
        if (!initialized_) return;

        std::cout << "[LuaScriptManager] Shutting down Lua VM..." << std::endl;

        if (runtimeActive_) stopRuntime();

        // Lua systems keep protected_function references into this VM. Release
        // them before any script environments or the Lua state are destroyed.
        // Keeping this cleanup here also makes project reloads and direct use of
        // LuaScriptManager safe without relying on EngineCore's call order.
        SystemManager::getInstance().clear();

        // Clean up entities created at script top-level as well. Runtime
        // entities have already been removed by stopRuntime().
        if (world_) {
            for (auto& scriptInfoPtr : scripts_) {
                for (uint64_t id : scriptInfoPtr->createdEntities) {
                    auto e = world_->entity(id);
                    if (e.is_alive()) e.destruct();
                }
                scriptInfoPtr->createdEntities.clear();
            }
        }

        // 2. Xóa bỏ mọi tham chiếu Lua
        for (auto& s : scripts_) {
            s->onInit = sol::nil;
            s->onUpdate = sol::nil;
            s->onShutdown = sol::nil;
            s->env = sol::environment();
        }
        scripts_.clear();
        
        lua_["ECS"] = sol::nil;
        lua_["Input"] = sol::nil;
        lua_["Physics"] = sol::nil;
        lua_["Camera"] = sol::nil;
        lua_["Scene"] = sol::nil;
        lua_["entities"] = sol::nil;

        // 3. Ép kiểu hủy máy ảo Lua
        try {
            lua_.collect_garbage();
            lua_ = sol::state(); 
        } catch(...) {}

        world_ = nullptr;
        ScriptComponentRegistry::getInstance().clear();
        initialized_ = false;
        runtimeActive_ = false;
        pendingScenePath_.reset();
        currentScenePath_.clear();
        std::cout << "[LuaScriptManager] Shutdown successful." << std::endl;
    }

    bool LuaScriptManager::loadScript(const std::string& scriptPath) {
        ScriptInfo* info = nullptr;
        for (auto& sPtr : scripts_) { if (sPtr->path == scriptPath) { info = sPtr.get(); break; } }
        if (!info) return false;

        // Tạo môi trường mới để đảm bảo tính cô lập
        info->env = sol::environment(lua_, sol::create, lua_.globals());
        
        // Thêm tính năng tự động theo dõi entity vào ECS.createEntity của riêng script này
        sol::table scriptEcs = lua_.create_table();
        sol::table globalEcs = lua_["ECS"];
        for (auto& pair : globalEcs) { scriptEcs[pair.first] = pair.second; }
        
        scriptEcs["createEntity"] = [this, info](sol::optional<std::string> name) -> uint64_t {
            if (!this->world_) return 0;
            auto e = name && !name->empty() ? this->world_->entity(name->c_str()) : this->world_->entity();
            uint64_t id = e.id();
            if (std::find(info->createdEntities.begin(), info->createdEntities.end(), id) == info->createdEntities.end()) {
                info->createdEntities.push_back(id);
            }
            return id;
        };
        info->env["ECS"] = scriptEcs;

        auto result = lua_.safe_script_file(scriptPath, info->env);
        if (!result.valid()) {
            sol::error err = result;
            std::cerr << "[Lua] Error loading " << scriptPath << ": " << err.what() << std::endl;
            return false;
        }

        info->onInit = info->env["onInit"];
        info->onUpdate = info->env["onUpdate"];
        info->onShutdown = info->env["onShutdown"];
        info->loaded = true;
        std::cout << "[LuaScriptManager] Loaded: " << scriptPath << std::endl;
        return true;
    }

    bool LuaScriptManager::reloadScript(const std::string& scriptPath) {
        ScriptInfo* info = nullptr;
        for (auto& sPtr : scripts_) { if (sPtr->path == scriptPath) { info = sPtr.get(); break; } }
        if (!info) return false;

        std::cout << "[LuaScriptManager] Reloading: " << scriptPath << std::endl;

        if (runtimeActive_ && info->loaded && info->onShutdown.valid()) {
            auto result = info->onShutdown();
            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "[Lua] Error in onShutdown before reload: " << err.what() << std::endl;
            }
        }

        if (world_) {
            for (uint64_t id : info->createdEntities) {
                auto e = world_->entity(id);
                if (e.is_alive()) e.destruct();
            }
            info->createdEntities.clear();
        }

        if (loadScript(scriptPath)) {
            if (runtimeActive_ && info->onInit.valid()) {
                auto res = info->onInit();
                if (!res.valid()) {
                    sol::error err = res;
                    std::cerr << "[Lua] Error in onInit after reload: " << err.what() << std::endl;
                }
            }
            return true;
        }
        return false;
    }

    void LuaScriptManager::scanScriptsFolder() {
        scripts_.clear();
        if (!std::filesystem::exists(scriptsFolder_)) return;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(scriptsFolder_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".lua") {
                std::string pathStr = entry.path().string();

                // Skip internal engine API and editor configs
                if (pathStr.find("levi-api") != std::string::npos) continue;
                if (pathStr.find("editor-configs") != std::string::npos) continue;
                if (pathStr.find(".git") != std::string::npos) continue;

                auto info = std::make_unique<ScriptInfo>();
                info->path = pathStr;
                info->lastModified = std::filesystem::last_write_time(entry.path());
                info->loaded = false;
                scripts_.push_back(std::move(info));
            }
        }
    }

    bool LuaScriptManager::loadAllScripts() {
        if (!initialized_) return false;
        scanScriptsFolder();
        for (auto& s : scripts_) loadScript(s->path);
        return true;
    }

    void LuaScriptManager::startRuntime() {
        if (!initialized_ || runtimeActive_) return;

        const bool deferred = world_ && world_->is_deferred();
        if (deferred) world_->defer_suspend();

        pendingScenePath_.reset();
        runtimeActive_ = true;
        callFunction("onInit");
        callFunction("onPlay");

        if (deferred) world_->defer_resume();
    }

    void LuaScriptManager::stopRuntime() {
        if (!initialized_ || !runtimeActive_) return;

        const bool deferred = world_ && world_->is_deferred();
        if (deferred) world_->defer_suspend();

        callFunction("onStop");
        callFunction("onShutdown");
        if (world_) {
            for (auto& script : scripts_) {
                for (uint64_t id : script->createdEntities) {
                    auto entity = world_->entity(id);
                    if (entity.is_alive()) entity.destruct();
                }
                script->createdEntities.clear();
            }
        }
        runtimeActive_ = false;
        pendingScenePath_.reset();

        if (deferred) world_->defer_resume();
    }

    std::optional<std::string> LuaScriptManager::takePendingSceneLoad() {
        auto request = std::move(pendingScenePath_);
        pendingScenePath_.reset();
        return request;
    }

    void LuaScriptManager::forgetCreatedEntities() {
        for (auto& script : scripts_) script->createdEntities.clear();
    }

    void LuaScriptManager::checkForChanges() {
        if (!initialized_) return;
        for (auto& s : scripts_) {
            if (!std::filesystem::exists(s->path)) continue;
            auto mt = std::filesystem::last_write_time(s->path);
            if (mt != s->lastModified) {
                s->lastModified = mt;
                reloadScript(s->path);
            }
        }
    }

    void LuaScriptManager::exportAPIDefinitions() {
        static const std::string projectConfig = R"json({
    "runtime.version": "Lua 5.4",
    "runtime.path": [
        "?.lua",
        "?/init.lua",
        "scripts/?.lua",
        "scripts/?/init.lua"
    ],
    "diagnostics.globals": [
        "ECS",
        "Input",
        "Physics",
        "Camera",
        "Scene",
        "PivotType",
        "entities"
    ],
    "workspace.library": [
        "scripts/levi-api"
    ],
    "workspace.checkThirdParty": false,
    "completion.callSnippet": "Both",
    "hint.enable": true,
    "hint.setType": true
}
)json";

        static const std::string scriptsConfig = R"json({
    "runtime.version": "Lua 5.4",
    "runtime.path": [
        "?.lua",
        "?/init.lua"
    ],
    "diagnostics.globals": [
        "ECS",
        "Input",
        "Physics",
        "Camera",
        "Scene",
        "PivotType",
        "entities"
    ],
    "workspace.library": [
        "levi-api"
    ],
    "workspace.checkThirdParty": false,
    "completion.callSnippet": "Both",
    "hint.enable": true,
    "hint.setType": true
}
)json";

        static const std::string apiDefinitions = R"lua(---@meta

---@class Vector2
---@field x number
---@field y number

---@class Position2D: Vector2
---@class Scale2D: Vector2

---@enum PivotType
PivotType = {
    Percent = 0,
    Pixel = 1
}

---@class Rotation2D
---@field angle number
---@field pivot Vector2
---@field pivotType PivotType

---@class Sprite2D
---@field texturePath string
---@field size Vector2

---@class AABBCollider2D
---@field size Vector2
---@field offset Vector2

---@class CircleCollider2D
---@field radius number
---@field offset Vector2

ECS = {}

---@param name? string
---@return integer id
function ECS.createEntity(name) end

---@param id integer
function ECS.deleteEntity(id) end

---@param id integer
---@param x number
---@param y number
function ECS.addPosition(id, x, y) end

---@param id integer
---@param x number
---@param y number
function ECS.setPosition(id, x, y) end

---@param id integer
---@return Position2D?
function ECS.getPosition(id) end

---@param id integer
---@param x number
---@param y number
function ECS.addScale(id, x, y) end

---@param id integer
---@param x number
---@param y number
function ECS.setScale(id, x, y) end

---@param id integer
---@return Scale2D?
function ECS.getScale(id) end

---@param id integer
---@param angle number
function ECS.addRotation(id, angle) end

---@param id integer
---@param angle number
function ECS.setRotation(id, angle) end

---@param id integer
---@param x number
---@param y number
---@param pivotType PivotType
function ECS.setRotationPivot(id, x, y, pivotType) end

---@param id integer
---@return Rotation2D?
function ECS.getRotation(id) end

---@param id integer
---@param path string
---@param width number
---@param height number
function ECS.addSprite(id, path, width, height) end

---@param id integer
---@return Sprite2D?
function ECS.getSprite(id) end

---@param id integer
---@param width number
---@param height number
function ECS.addAABBCollider(id, width, height) end

---@param id integer
---@param radius number
function ECS.addCircleCollider(id, radius) end

---@param id integer
---@param zoom? number
function ECS.addCamera(id, zoom) end

---@param id integer
---@return Camera2D?
function ECS.getCamera(id) end

---@param path string
---@return table
function ECS.AssetPath(path) end

---@param name string
---@param defaultValues table
function ECS.defineComponent(name, defaultValues) end

---@param id integer
---@param schemaName string
function ECS.addComponent(id, schemaName) end

---@param id integer
---@param schemaName string
---@param fieldName string
---@param value any
function ECS.setComponentValue(id, schemaName, fieldName, value) end

---@param id integer
---@param schemaName string
---@param fieldName string
---@return any
function ECS.getComponentValue(id, schemaName, fieldName) end

---@param name string
---@param query string[]
---@param callback fun(entityId: integer)
function ECS.registerSystem(name, query, callback) end

Input = {}

---@param key string
---@return boolean
function Input.isKeyDown(key) end

---@param key string
---@return boolean
function Input.isKeyPressed(key) end

---@param key string
---@return boolean
function Input.isKeyReleased(key) end

---@param button integer
---@return boolean
function Input.isMouseButtonDown(button) end

---@param button integer
---@return boolean
function Input.isMouseButtonPressed(button) end

---@return Vector2
function Input.getMousePosition() end

Physics = {}

---@param first integer
---@param second integer
---@return boolean
function Physics.overlaps(first, second) end

---@param ax number
---@param ay number
---@param aw number
---@param ah number
---@param bx number
---@param by number
---@param bw number
---@param bh number
---@return boolean
function Physics.overlapsAABB(ax, ay, aw, ah, bx, by, bw, bh) end

---@param ax number
---@param ay number
---@param ar number
---@param bx number
---@param by number
---@param br number
---@return boolean
function Physics.overlapsCircle(ax, ay, ar, bx, by, br) end

Camera = {}

---@return integer?
function Camera.getActive() end

---@param id integer
---@return boolean
function Camera.setActive(id) end

---@param deltaX number
---@param deltaY number
---@return boolean
function Camera.move(deltaX, deltaY) end

---@param x number
---@param y number
---@return boolean
function Camera.setPosition(x, y) end

---@param zoom number
---@return boolean
function Camera.setZoom(zoom) end

---@return number
function Camera.getZoom() end

---@param intensity number
---@param duration number
---@return boolean
function Camera.shake(intensity, duration) end

---@class Camera2D
---@field zoom number
---@field active boolean
---@field maxShakeOffset number
---@field maxShakeRotation number

Scene = {}

---Queue a scene switch at the end of the current simulation frame.
---@param path string Project-relative .levscene.json or .levscene.bin path
---@return boolean queued
function Scene.load(path) end

---Queue a reload of the active scene.
---@return boolean queued
function Scene.reload() end

---Return the active scene path, relative to the project when possible.
---@return string
function Scene.current() end
)lua";

        try {
            const std::filesystem::path projectRoot(projectPath_);
            const std::filesystem::path scriptsRoot(scriptsFolder_);
            writeTextFileIfChanged(projectRoot / ".luarc.json", projectConfig);
            writeTextFileIfChanged(scriptsRoot / ".luarc.json", scriptsConfig);
            writeTextFileIfChanged(scriptsRoot / "levi-api" / "ecs.lua", apiDefinitions);
            std::cout << "[LuaScriptManager] LuaLS workspace files are ready in: "
                      << scriptsRoot << std::endl;
        } catch (const std::exception& e) {
            lastError_ = e.what();
            std::cerr << "[LuaScriptManager] LSP export warning: " << e.what() << std::endl;
        }
    }
}
