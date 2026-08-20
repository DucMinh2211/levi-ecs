# 📝 Levi-ECS Engine - Roadmap & TODO

Progress tracking for the Levi-ECS project, categorized by implementation status.

## 🔴 Critical Issues (High Priority)
- [x] **Fix Engine Shutdown Crash:** Lua callbacks/environments are released before the Lua VM, and the Lua manager is destroyed before the Flecs world. Covered by a shutdown/re-init lifecycle test.
- [x] **Undo/Redo System:** Essential for editor productivity (tracking Inspector/Hierarchy changes).
    - [x] Command history core with bounded undo/redo stacks and redo invalidation.
    - [x] Editor menu and `Ctrl+Z` / `Ctrl+Y` shortcuts.
    - [x] Inspector edits for built-in and Lua components, rename, and add-component actions.
    - [x] Hierarchy create entity/create child actions.
    - [x] Snapshot and restore deleted entity subtrees, including hierarchy, built-in components, and Lua component values.

## 📊 Current Status (2026-08-20)
- [x] Windows CMake/Ninja build restored; Objective-C is now enabled only on macOS.
- [x] Automated lifecycle, undo/redo core, and hierarchy deletion tests added and passing.
- [x] Undo/Redo milestone completed, including deletion and restoration of entity subtrees.
- [x] Short-term feature polish completed: scene persistence, play controls, ECS modules, asset thumbnails/drag-drop, Lua input, and base collision APIs.
- [x] 2D Camera completed across rendering, Lua, Inspector, persistence, and automated tests.
- [ ] Next focus: full Box2D-backed 2D Physics.

## ✅ 1. Completed (Core Foundation & Stability)
- [x] **Engine Core Initialization:** SDL3 Integration (Windowing & Renderer).
- [x] **Flecs v4 Upgrade & Stability:** 
    - [x] Fixed pair usage with strings (entity-based pairs).
    - [x] Refactored to `query_builder` API.
    - [x] **LOCKED_STORAGE Fix:** Implemented global `defer_begin/end` in the main loop to prevent crashes during world iteration.
- [x] **Flexible Scripting Architecture:**
    - [x] **Project-wide Scanning:** Any `.lua` file in the project (any folder depth) is detected.
    - [x] **Modular Require:** Configured `package.path` to support `require` from project root.
    - [x] **Isolation:** `sol::environment` per script with automatic entity tracking/cleanup.
- [x] **Editor Tooling:** 
    - [x] **Scene Hierarchy:** Create/Delete entities, Create Children (Parent-Child support).
    - [x] **Inspector:** Dynamic editing of C++ and Lua components.
    - [x] **Project Explorer:** "Copy as path" and "Copy relative path" context menus.
- [x] **Asset Manager:** Singleton to manage loading/unloading of `SDL_Texture`.

## 🛠️ 2. Short-term (Feature Polish)
- [x] **Scene Persistence:**
    - [x] **JSON Export:** Human-readable, versioned scene save/load.
    - [x] **Binary Export:** Compact, versioned scene save/load for production data.
- [x] **Editor Play/Pause/Stop Mode:**
    - [x] Control simulation time and Lua updates from the Editor UI.
    - [x] Snapshot edit state on Play and restore it on Stop.
- [x] **Modular C++ ECS Modules:** Render and Transform are reusable Flecs modules (`world.import<T>`).
- [x] **Asset Browser Improvements:** Texture thumbnails, file grid, and asset-path drag-and-drop into Inspector.
- [x] **Lua API Expansion:**
    - [x] **Input:** Keyboard and mouse state/edge wrappers exposed to Lua.
    - [x] **Physics Base:** AABB and Circle collider components and overlap API.

## ⏳ 3. Mid-term (Advanced Systems)
- [x] **2D Camera:**
    - [x] Active-camera world-to-screen rendering with movement and zoom.
    - [x] Time-decaying positional and rotational screen shake.
    - [x] Lua API, Inspector/Undo support, and JSON/binary persistence.
- [ ] **2D Physics:** Full Box2D integration for collisions and gravity.
- [ ] **Animation System:** Support for Sprite Sheets and frame-based animations.
- [ ] **Tilemap Editor:** Grid-based mapping tool directly inside the Editor.

## 🚀 4. Long-term Vision (Post v1.0.0)
- [ ] **Standalone Game Export (Publishing):**
    - [ ] **Player Executable:** A lightweight, optimized engine build without ImGui and Editor code.
    - [ ] **Project Bundler:** Tool to package all Assets, Scripts, and Binary Scene data into a single encrypted or compressed binary file.
    - [ ] **One-Click Export:** Editor UI to select target platform and generate the final game executable.
- [ ] **Advanced 2D Lighting:** Dynamic lights and shadow mapping.
- [ ] **Particle System:** Professional editor for smoke, fire, and explosion effects.
- [ ] **Web Export:** Support for WebAssembly (Emscripten) compilation.
