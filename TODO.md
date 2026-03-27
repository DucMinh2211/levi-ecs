# 📝 Levi-ECS Engine - Roadmap & TODO

Progress tracking for the Levi-ECS project, categorized by implementation status.

## ✅ 1. Completed (Core Foundation & Basic Rendering)
The foundational components are operational and stable.
- [x] **Engine Core Initialization:** SDL3 Integration (Windowing & Renderer).
- [x] **ECS Integration:** Flecs (v4) embedded into the main engine loop.
- [x] **Basic Math Library:** Defined `Vector2` and `Vector3` types for future-proofing.
- [x] **2D Component System:** Implemented `Position2D`, `Scale2D`, `Rotation2D`, and `Sprite2D`.
- [x] **Basic Render System:** 
    - [x] ECS system to query and render entities.
    - [x] Placeholder rendering (colored rectangles) for entities with sprites.
- [x] **Viewport Rendering (Render to Texture):**
    - [x] Game logic renders to a dedicated virtual texture.
    - [x] Editor displays the game view within a dedicated ImGui "Viewport" window.
- [x] **Editor UI Base:** Integrated Dear ImGui with Docking support.
- [x] **Build System:** `build.ps1` script and cross-platform CMake configuration.
- [x] **Architecture:** Clean separation between `engine/` (core) and `editor/` (studio).
- [x] **Asset Manager:**
    - [x] Singleton to manage loading/unloading of Textures (SDL_Texture).
    - [x] Support for actual image loading (integrated SDL3_image).
- [x] **Editor Tooling (Phase 1):**
    - [x] **Project Explorer:** File system navigation and native path resolution.

## 🛠️ 2. Short-term (Core Features)
High-priority features to complete the basic development workflow.
- [ ] **Hot-reloading Mechanism (`cr.h`):**
    - [ ] Decouple game logic into a separate DLL/SO.
    - [ ] Real-time reloading of logic within the Editor without restarting.
- [ ] **Editor Tooling (Phase 2):**
    - [ ] **Scene Hierarchy:** List all entities currently in the ECS world.
    - [ ] **Inspector:** Basic UI to modify `Position2D`, `Scale2D`, etc., in real-time.

## ⏳ 3. Mid-term (Advanced Systems)
Essential systems required to build a functional game.
- [ ] **2D Camera:** Support for movement, zooming, and screen shake.
- [ ] **Input System:** Wrapper for Keyboard, Mouse, and Gamepad input.
- [ ] **2D Physics:** Integrate Box2D for collisions and gravity.
- [ ] **Animation System:** Support for Sprite Sheets and frame-based animations.
- [ ] **Tilemap Editor:** Grid-based mapping tool directly inside the Editor.
- [ ] **Audio System:** Integrate SDL_mixer for Background Music (BGM) and Sound Effects (SFX).
- [ ] **Scene Persistence:** Save and load scenes using JSON or YAML.

## 🚀 4. Long-term Vision (Post v1.0.0)
Scaling features for a mature engine.
- [ ] **Advanced 2D Lighting:** Dynamic lights and shadow mapping.
- [ ] **Particle System:** Professional editor for smoke, fire, and explosion effects.
- [ ] **Scripting Language:** Lua or C# integration for high-level logic.
- [ ] **AI Pathfinding:** Navmesh or A* systems for 2D entities.
- [ ] **Visual Scripting:** Node-based logic editor for non-programmers.
- [ ] **Web Export:** Support for WebAssembly (Emscripten) compilation.
- [ ] **Standalone Build Tool:** Automated packaging for distribution (removing Editor code).
