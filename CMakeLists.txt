cmake_minimum_required(VERSION 3.24)
project(Levi-ECS VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile_commands.json for LSP (Clangd/Neovim)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output Directories (Must be set BEFORE add_subdirectory to apply to all targets)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_CURRENT_SOURCE_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_CURRENT_SOURCE_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_CURRENT_SOURCE_DIR}/lib)
endforeach()

# === Third-party Dependencies (Vendored) ===

# --- SDL3 ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/SDL3)

# --- SDL_image ---
set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDLIMAGE_JPG ON CACHE BOOL "" FORCE)
set(SDLIMAGE_PNG ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/SDL_image)

# --- Flecs (ECS) ---
set(FLECS_STATIC ON CACHE BOOL "" FORCE)
set(FLECS_SHARED OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/flecs)

# --- Native File Dialog Extended (NFD) ---
set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/nfd)

# --- Dear ImGui (Built manually) ---
add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/backends/imgui_impl_sdl3.cpp
    third_party/imgui/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui PUBLIC 
    third_party/imgui
    third_party/imgui/backends
)
target_compile_definitions(imgui PUBLIC SDL_ENABLE_OLD_NAMES)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# --- Lua ---
file(GLOB LUA_SOURCES "third_party/lua/*.c")
list(REMOVE_ITEM LUA_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua/lua.c" 
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua/luac.c" 
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua/onelua.c"
)
add_library(liblua STATIC ${LUA_SOURCES})
target_include_directories(liblua PUBLIC third_party/lua)

# --- sol2 (Header-only) ---
add_library(sol2 INTERFACE)
target_include_directories(sol2 INTERFACE third_party/sol2/include)

# Define directories
set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/engine)
set(EDITOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/editor)

# Add subdirectories
add_subdirectory(engine)
add_subdirectory(editor)
