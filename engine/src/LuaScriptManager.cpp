#include "Levi/LuaScriptManager.h"
#include "Levi/Components.h"
#include "Levi/Math.h"
#include "Levi/SystemManager.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <algorithm>

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

        projectPath_ = projectPath;
        scriptsFolder_ = (std::filesystem::path(projectPath) / "scripts").string();
        world_ = world;

        if (!world_) return false;

        try {
            lua_ = sol::state();
            lua_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::string, sol::lib::table);

            bindECS(world_);
            
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

        // --- Dynamic Script Components (ECS Abstraction Phase 1) ---

        ecs["defineComponent"] = [](std::string name, sol::table defaultValues) {
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
                
                schema.fields.push_back({fieldName, type});
            }
            
            ScriptComponentRegistry::getInstance().registerSchema(schema);
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
    }

    void LuaScriptManager::shutdown() {
        if (!initialized_) return;

        std::cout << "[LuaScriptManager] Shutting down Lua VM..." << std::endl;

        // 1. Dọn dẹp entities của các script trước
        if (world_) {
            for (auto& scriptInfoPtr : scripts_) {
                if (scriptInfoPtr->loaded && scriptInfoPtr->onShutdown.valid()) {
                    try { scriptInfoPtr->onShutdown(); } catch(...) {}
                }
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
        lua_["entities"] = sol::nil;

        // 3. Ép kiểu hủy máy ảo Lua
        try {
            lua_.collect_garbage();
            lua_ = sol::state(); 
        } catch(...) {}

        world_ = nullptr;
        initialized_ = false;
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

        if (info->loaded && info->onShutdown.valid()) { try { info->onShutdown(); } catch(...) {} }

        if (world_) {
            for (uint64_t id : info->createdEntities) {
                auto e = world_->entity(id);
                if (e.is_alive()) e.destruct();
            }
            info->createdEntities.clear();
        }

        if (loadScript(scriptPath)) {
            if (info->onInit.valid()) {
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
                if (entry.path().string().find("levi-api") != std::string::npos) continue;
                auto info = std::make_unique<ScriptInfo>();
                info->path = entry.path().string();
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
        for (auto& s : scripts_) {
            if (s->loaded && s->onInit.valid()) {
                auto res = s->onInit();
                if (!res.valid()) {
                    sol::error err = res;
                    std::cerr << "[Lua] Error in onInit: " << err.what() << std::endl;
                }
            }
        }
        return true;
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
        // LSP generation...
    }
}
