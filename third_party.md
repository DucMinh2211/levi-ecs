# Third-Party Dependencies (Vendored)

All dependencies are vendored in `third_party/` for offline builds and stability.

- **SDL3** - https://github.com/libsdl-org/SDL (main branch)
- **SDL_image** - https://github.com/libsdl-org/SDL_image (main branch)  
- **Flecs** - https://github.com/SanderMertens/flecs (v4.0.0)
- **ImGui** - https://github.com/ocornut/imgui (v1.91.0-docking)
- **NFD Extended** - https://github.com/btzy/nativefiledialog-extended (v1.2.1)
- **Lua** - https://github.com/lua/lua (v5.4.6)
- **sol2** - https://github.com/ThePhD/sol2 (v3.3.0)

## Build System

CMake uses `add_subdirectory()` to build dependencies from source.
No internet required after initial clone.
