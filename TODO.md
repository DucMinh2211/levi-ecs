# 📝 Levi-ECS Engine - Roadmap & TODO

Progress tracking for the Levi-ECS project, categorized by implementation status.

## 🔴 Critical Issues (High Priority)
- [ ] **Fix Engine Shutdown Crash:** Resolve `fatal: poly.c: 59: assert: hdr->magic == ECS_OBJECT_MAGIC` occurring when closing the editor. (Destruction order conflict between Lua GC and Flecs World).
- [ ] **Undo/Redo System:** Essential for editor productivity (tracking Inspector/Hierarchy changes).

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
- [ ] **Scene Persistence:**
    - [ ] **JSON/YAML Export:** For human-readable scene editing.
    - [ ] **Binary Export:** For optimized, fast-loading production data.
- [ ] **Editor Play/Pause/Stop Mode:** 
    - [ ] Control `world.progress()` and Lua updates from the Editor UI.
    - [ ] Reset scene state when stopping.
- [ ] **Modular C++ ECS Modules:** Refactor core systems (Render, Transform) into reusable Flecs modules (`world.import<T>`).
- [ ] **Asset Browser Improvements:** Better UI for browsing textures/sounds (thumbnails, drag-and-drop into Inspector).
- [ ] **Lua API Expansion:** 
    - [ ] **Input:** Wrapper for Keyboard/Mouse (exposed to Lua).
    - [ ] **Physics Base:** Simple AABB or Circle collision API.

## ⏳ 3. Mid-term (Advanced Systems)
- [ ] **2D Camera:** Support for movement, zooming, and screen shake.
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
