# 📝 Levi-ECS Engine - Roadmap & TODO

Progress tracking for the Levi-ECS project, categorized by implementation status.

## 🔴 Critical Issues
- [x] **Fix Engine Shutdown Crash:** Lua callbacks/environments are released before the Lua VM, and the Lua manager is destroyed before the Flecs world. Covered by a shutdown/re-init lifecycle test.
- [x] **Undo/Redo System:** Essential for editor productivity (tracking Inspector/Hierarchy changes).
    - [x] Command history core with bounded undo/redo stacks and redo invalidation.
    - [x] Editor menu and `Ctrl+Z` / `Ctrl+Y` shortcuts.
    - [x] Inspector edits for built-in and Lua components, rename, and add-component actions.
    - [x] Hierarchy create entity/create child actions.
    - [x] Snapshot and restore deleted entity subtrees, including hierarchy, built-in components, and Lua component values.
- [ ] **Make SDL Resource Teardown and Partial Initialization Safe:** `AssetManager` currently outlives the renderer during `EngineCore` teardown, while failed initialization paths leave already-created SDL/ImGui resources allocated.
    - [ ] Clear renderer-owned textures before destroying the viewport texture and renderer; make ownership and destruction order explicit.
    - [ ] Roll back every successful initialization stage when a later SDL window/renderer/texture or ImGui backend stage fails.
    - [ ] Make shutdown idempotent independently of `isRunning_` so partially initialized engines are also cleaned up.
    - [ ] Add fault-injection lifecycle tests for every initialization failure point and repeated shutdown.
- [ ] **Prevent Lua Scripts from Owning or Deleting Pre-existing Scene Entities:** Script-local `ECS.createEntity(name)` currently tracks an existing named Flecs entity as newly created and destroys it on Stop or hot reload.
    - [ ] Separate explicit `spawn/create` from `find/lookup`, and only grant ownership when a new entity was actually created.
    - [ ] Track ownership with generation-safe handles or an ECS ownership relation instead of an unqualified vector of numeric IDs.
    - [ ] Define behavior for duplicate names and cross-script ownership rather than silently sharing and later deleting the same entity.
    - [ ] Add regression tests where runtime scripts request names already used by scene entities and other scripts.
- [ ] **Remove Shell Command Injection from External File Opening:** macOS/Linux `ProjectExplorer::openInSystem` concatenates project-controlled paths into `system("open/xdg-open ...")` commands.
    - [ ] Replace shell construction with a native process API or a safe platform abstraction that passes the path as an argument.
    - [ ] Cover spaces, quotes, shell metacharacters, Unicode, and non-existent paths in platform tests.

## 🟠 High-Priority Architecture and Correctness
- [ ] **Remove Hard-Coded Component Snapshots:** Make entity deletion and scene persistence extensible without updating a list of `std::optional<T>` fields for every new component.
    - [ ] Replace `DeleteEntityCommand`'s built-in component enumeration with a generic snapshot mechanism or editor tombstone/soft-delete lifecycle.
    - [ ] Introduce one component metadata/codec registry (or Flecs reflection where appropriate) shared by serialization, editor rendering, and Lua query resolution.
    - [ ] Represent Lua-defined components generically as `(ScriptComponent, schema)` data and restore schemas by stable name/GUID instead of transient Flecs entity IDs.
    - [ ] Preserve supported tags, pairs, and relationships instead of only component data plus `ChildOf`.
    - [ ] Replace name-based scene-entity heuristics with explicit scene ownership; names such as `World`, `$foo`, or a Lua schema name must not disappear from hierarchy/save data.
    - [ ] Replace raw native-layout binary writes with a defined byte order and field-wise schema; add size/count limits, complete-input checks, format migration, and unknown-component handling.
    - [ ] Save through a temporary file plus atomic replace/backup so a failed write cannot truncate the active scene.
    - [ ] Add tests proving newly registered native/Lua components, relationships, unusual names, malformed files, and cross-build binary files round-trip safely.
- [ ] **Harden and Decouple the Inspector:** Fix correctness issues in component editing, then remove the one-render-block-per-native-component architecture.
    - [ ] Make `Active Camera` use the exclusive camera activation API and preserve the complete previous active-camera state for undo/redo.
    - [ ] Initialize missing Lua fields from their declared `ScriptFieldType` and schema default value instead of implicitly inserting `float{0}`.
    - [ ] Scope ImGui IDs by Lua schema/component so fields with the same name cannot collide across components.
    - [ ] Record asset-path text edits and drag/drop changes consistently in undo/redo history.
    - [ ] Replace string-prefix project path checks with normalized path-component checks; remove fixed-size name/path edit buffers.
    - [ ] Avoid no-op edit commands and clean stale interactive edit state when the selected entity or component changes.
    - [ ] Introduce a `ComponentEditorRegistry` for native component rendering, add/remove actions, dependencies, and undo setters.
    - [ ] Add automated Inspector interaction tests for native fields, Lua field types/IDs, camera exclusivity, and asset drag/drop undo.
- [ ] **Make Project Switching a Single Transaction:** The Project Explorer, `EngineCore`, Lua VM, scene, physics, asset base path/cache, editor history, and layout can currently point at different projects.
    - [ ] Route every project-root edit, folder picker, and menu action through one validated `openProject` operation.
    - [ ] Stage and validate the new project before committing; retain the old project if Lua initialization or project loading fails.
    - [ ] Explicitly unload the old scene, script schema metadata, physics runtime, cached assets, selection, and undo history on a successful switch.
    - [ ] Add two-project switching tests that verify no entities, textures, systems, schemas, scene paths, or callbacks leak across project contexts.
- [ ] **Make Lua Hot Reload Transactional and Ownership-aware:** Reload currently destroys old runtime state before proving the new script compiles, misses added/deleted files, and leaves systems that were removed or renamed by a script.
    - [ ] Compile/execute into a staging environment and swap callbacks/systems only after success; keep the last good script active after syntax/runtime load errors.
    - [ ] Rescan or watch for added, removed, and renamed Lua files, invoking shutdown and cleanup for deleted scripts.
    - [ ] Track Lua systems by owning script so reload/unload removes stale registrations without name collisions across scripts.
    - [ ] Define schema evolution/default-value migration for existing `ScriptComponent` instances after a component definition changes.
    - [ ] Quarantine or rate-limit repeatedly failing callbacks instead of logging the same error every frame.
    - [ ] Add tests for failed reload, new/deleted files, renamed systems, duplicate system names, and schema migration.
- [ ] **Scope Runtime Registries to an Engine/World:** `SystemManager` and `ScriptComponentRegistry` are process-global singletons even though their callbacks, schemas, and Flecs IDs belong to one Lua VM/world.
    - [ ] Store registries in the owning `EngineCore`/`LuaScriptManager` or as world context/singletons with explicit lifetime.
    - [ ] Remove mutable global access and make multi-world/re-init behavior deterministic and independently testable.
- [ ] **Validate and Normalize the Lua ECS Boundary:** Several setters accept dead/stale entity IDs, schema names and fields are not validated, numeric Lua defaults can be classified as floats before integers, and sprite paths become machine-specific absolute paths.
    - [ ] Validate entity liveness/generation before every mutation and return structured failure information consistently.
    - [ ] Namespace schema entities and reject collisions with native components, modules, systems, or scene entity names.
    - [ ] Validate fields against schema and enforce declared types, including correct Lua integer detection and stored default values.
    - [ ] Keep asset references project-relative and normalized across Lua, Inspector, serializer, and `AssetManager`.
    - [ ] Validate Lua system query component names instead of silently creating a pair target for typos.
- [ ] **Preserve Entity Identity Across Create Undo/Redo:** Redo of `Create Entity/Child` allocates a new Flecs ID, while later edit commands still capture the original handle and silently no-op.
    - [ ] Restore the same generation-safe identity or introduce stable editor entity GUIDs used by every command.
    - [ ] Test create → edit/add component → undo to before creation → redo the entire chain for roots and children.
- [ ] **Fix Scene Hierarchy Selection Semantics:** The background `IsMouseDown && IsWindowHovered` path can clear selection in the same frame that an entity node is clicked.
    - [ ] Clear selection only when the click did not target an item, popup, drag source, or tree control.
    - [ ] Add interaction tests for select, expand, context menu, create child, delete, and empty-background deselect.
- [ ] **Separate Runtime Engine from the ImGui Editor Host:** `LeviEngine` directly initializes and renders ImGui and exposes a UI callback, preventing the planned standalone player from using a lightweight runtime core.
    - [ ] Split platform/window/render loop services from editor docking/UI orchestration and keep ImGui out of the runtime engine target.
    - [ ] Define explicit edit, simulation, render, and scene-transition phases instead of embedding editor work inside `EngineCore::run`.

## 🟡 Medium-Priority Robustness and Performance
- [ ] **Make Frame Timing and Simulation Order Explicit:** Lua `onUpdate` receives `world_.delta_time()` before the current `world.progress()` call, and physics/contact events are observed one frame later by construction.
    - [ ] Measure frame delta once, pass the same bounded value to Lua and Flecs, and document/order script, physics, contact dispatch, camera, and render phases.
    - [ ] Add deterministic pause/resume, first-play-frame, long-frame, and scene-switch timing tests.
- [ ] **Build a Coherent Asset Cache Lifecycle:** Relative loads are cached under resolved paths while `unloadTexture` searches the raw input, failed loads retry/log every frame, file changes never invalidate textures, and project switches retain old assets.
    - [ ] Use one canonical project-relative asset key and explicit fallback/error assets.
    - [ ] Add timestamp/version invalidation, negative-cache throttling, and clear/rebind behavior on renderer or project changes.
    - [ ] Move disk decode/GPU upload out of the per-entity render query or provide an asynchronous/preload path.
- [ ] **Remove Per-sprite Render Overhead and Viewport Distortion:** Output-size queries and cache lookups occur per sprite, while a fixed 1920×1080 texture is stretched to arbitrary ImGui viewport dimensions and input remains in window coordinates.
    - [ ] Compute frame render context once, resize the render target safely, preserve aspect ratio, and map mouse input into viewport/world coordinates.
    - [ ] Define behavior for negative scale, missing textures, render ordering/layers, and camera rotation/pivot interaction.
- [ ] **Complete and Validate Physics Semantics:** Manual `overlaps` ignores circle-box and rotated shapes, runtime reset discards configured gravity, and component values can enter invalid/NaN states outside Inspector clamps.
    - [ ] Either implement all supported shape combinations/transforms or rename/document the helper as a limited approximation.
    - [ ] Decide whether gravity is project/scene/runtime state and preserve or deliberately reinitialize it during Play/Stop and scene changes.
    - [ ] Centralize validation for finite transforms, positive dimensions, damping/material ranges, enum values, and collision filters.
- [ ] **Stop Swallowing Filesystem and Platform Errors:** Project Explorer catches directory exceptions without reporting them, project/Lua loading returns incomplete success information, and NFD/SDL/ImGui backend return values are not consistently checked.
    - [ ] Introduce structured errors with operation/path context and surface them in the editor instead of only `stdout/stderr`.
    - [ ] Distinguish complete, partial, and failed script/project loads; make unused `ScriptInfo::errorMessage` and `lastError_` state authoritative or remove them.
- [ ] **Harden Scene Parsing and Validation:** The custom JSON parser has incomplete Unicode escape/number handling, scene values and parent indices are weakly validated, and binary counts can request unreasonable allocations.
    - [ ] Use a tested parser or fully validate UTF-8/escapes/numbers, finite numeric ranges, enum values, hierarchy topology, duplicate names/IDs, and allocation budgets before mutating the world.
    - [ ] Fuzz JSON and binary loaders and verify failed loads leave the current scene unchanged.
- [ ] **Add Automated Quality Gates:** Current tests cover happy-path lifecycle/serialization/physics but not GUI behavior, renderer resources, project switching, hot-reload failure, malformed persistence, or platform launch behavior.
    - [ ] Add compiler warnings for project-owned targets, sanitizer builds where supported, format-check (non-mutating) CI, and platform build/test jobs.
    - [ ] Keep mutating `clang-format` separate from a CI `clang-format --dry-run --Werror` target.
- [ ] **Make Build Outputs and Helper Scripts Reproducible:** All build trees/configurations write into shared source `bin/lib` directories, and Ninja helper builds do not set `CMAKE_BUILD_TYPE` despite claiming Debug.
    - [ ] Keep artifacts per build tree/configuration and avoid Debug/Release or concurrent build collisions.
    - [ ] Make helper scripts handle generator changes, fail fast, set an explicit build type, and report missing platform dependencies consistently.

## 🟢 Low-Priority Maintainability
- [ ] **Make Editor and Serialization Ordering Deterministic:** Sort hierarchy roots/children, project files, Lua scripts, schemas, fields, and systems where order affects UI, output diffs, or load behavior.
- [ ] **Remove Fixed-size UI Buffers and Transitive Includes:** Replace project/name/ImGui ini character arrays with string-backed APIs, guarantee termination where arrays remain, move Windows headers out of public headers, and include every directly used standard header.
- [ ] **Reduce Repeated Runtime Query Construction:** Cache or own long-lived Flecs queries for active cameras, physics bodies, and Lua systems, with explicit invalidation on schema/system changes.
- [ ] **Synchronize Documentation with Implemented Behavior:** Remove unsupported feature claims, document binary portability/version limits until fixed, and update stale file/line references and test counts in lifecycle documentation.

## 📊 Current Status (2026-08-25)
- [x] Project-owned architecture and implementation audited across engine, editor, Lua scripts, persistence, build tooling, and tests; findings ranked above by severity (`third_party` excluded).
- [x] Windows CMake/Ninja build restored; Objective-C is now enabled only on macOS.
- [x] Automated lifecycle, undo/redo core, and hierarchy deletion tests added and passing.
- [x] Undo/Redo milestone completed, including deletion and restoration of entity subtrees.
- [x] Short-term feature polish completed: scene persistence, play controls, ECS modules, asset thumbnails/drag-drop, Lua input, and base collision APIs.
- [x] 2D Camera completed across rendering, Lua, Inspector, persistence, and automated tests.
- [x] Box2D 3.1.1 physics completed with rigid bodies, gravity, collision events, editor/persistence, and a playable Lua demo.
- [ ] Next focus: sprite-sheet Animation System.

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
- [x] **2D Physics:**
    - [x] Vendored Box2D 3.1.1 with fixed-step gravity and static/kinematic/dynamic bodies.
    - [x] AABB/circle shapes, material settings, sensors, filters, forces, impulses, and contact events.
    - [x] Lua API, Inspector/Undo support, JSON/binary persistence, lifecycle reset, tests, and demo.
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
