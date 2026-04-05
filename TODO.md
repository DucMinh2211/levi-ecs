# 📝 Levi-ECS Engine - Roadmap & TODO

Progress tracking for the Levi-ECS project, categorized by implementation status.

## 🔴 Critical Issues (High Priority)
- [ ] **Fix Engine Shutdown Crash:** Resolve `fatal: poly.c: 59: assert: hdr->magic == ECS_OBJECT_MAGIC` occurring when closing the editor. This is a destruction order conflict between Lua GC and Flecs World.
- [ ] **Restore Viewport/Hierarchy Stability:** Ensure API name consistency between C++ and Lua to prevent empty viewports after hot-reloading.

## ✅ 1. Completed (Core Foundation & Scripting)
The foundational components are operational and stable.
- [x] **Engine Core Initialization:** SDL3 Integration (Windowing & Renderer).
- [x] **ECS Integration:** Flecs (v4) embedded into the main engine loop.
- [x] **Basic Math Library:** Defined `Vector2` and `Vector3` types.
- [x] **2D Component System:** Implemented `Position2D`, `Scale2D`, `Rotation2D`, and `Sprite2D`.
- [x] **Basic Render System:** ECS system to query and render entities via SDL3_image.
- [x] **Viewport Rendering:** Game logic renders to a dedicated virtual texture displayed in ImGui.
- [x] **Editor UI Base:** Integrated Dear ImGui with Docking support.
- [x] **Build System:** `build.ps1` script and cross-platform CMake configuration.
- [x] **Asset Manager:** Singleton to manage loading/unloading of `SDL_Texture`.
- [x] **Editor Tooling:** Project Explorer, Scene Hierarchy, and Inspector.
- [x] **Lua Integration & Hot-reloading:**
    - [x] Integrate Lua runtime and sol2 bindings.
    - [x] **Per-script environment isolation:** Using `sol::environment` to prevent global namespace pollution.
    - [x] **Automatic Entity Tracking:** Each script now tracks its own created entities for guaranteed cleanup.
    - [x] **Reliable Hot-reload:** Automatically call `onShutdown`, delete old entities, and re-run `onInit` when scripts change.
    - [x] Auto-generate Lua type definitions for LSP support (autocomplete).
- [x] **Editor UX Improvements:** Native file dialogs, docking layouts, and project-specific configurations.

## 🛠️ 2. Short-term (Core Features)
- [ ] **Unified Component Management:** Resolve inconsistency between adding components via code vs. Editor Inspector.
    - *Proposed Path:* Components and Systems are defined in Lua scripts, but attaching/configuring them on entities should be handled primarily through the Inspector. This requires robust entity state persistence.
- [ ] **Lua Script Editor:** In-engine Lua script viewer/editor with syntax highlighting.
- [ ] **Refine Build Stability:** Fix SDL_image build issues (switching to stable tags/releases).
- [ ] **Component Hot-reload in Inspector:** Allow editing components in Inspector and see changes immediately.

## ⏳ 3. Mid-term (Advanced Systems)
- [ ] **2D Camera:** Support for movement, zooming, and screen shake.
- [ ] **Input System:** Wrapper for Keyboard, Mouse, and Gamepad input (exposed to Lua).
- [ ] **2D Physics:** Integrate Box2D for collisions and gravity.
- [ ] **Animation System:** Support for Sprite Sheets and frame-based animations.
- [ ] **Tilemap Editor:** Grid-based mapping tool directly inside the Editor.
- [ ] **Scene Persistence:** Save and load scenes using JSON or YAML.

## 🚀 4. Long-term Vision (Post v1.0.0)
- [ ] **Advanced 2D Lighting:** Dynamic lights and shadow mapping.
- [ ] **Particle System:** Professional editor for smoke, fire, and explosion effects.
- [ ] **AI Pathfinding:** Navmesh or A* systems for 2D entities.
- [ ] **Web Export:** Support for WebAssembly (Emscripten) compilation.
- [ ] **Standalone Build Tool:** Automated packaging for distribution.
