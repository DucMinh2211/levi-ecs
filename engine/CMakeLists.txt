add_library(LeviEngine STATIC
    src/Engine.cpp
    src/AssetManager.cpp
    src/LuaScriptManager.cpp
    src/Input.cpp
    src/Camera2D.cpp
    src/Physics2D.cpp
    src/Modules.cpp
    src/SceneSerializer.cpp
    include/Levi/Engine.h
    include/Levi/AssetManager.h
    include/Levi/LuaScriptManager.h
    include/Levi/Math.h
    include/Levi/Components.h
    include/Levi/Input.h
    include/Levi/Camera2D.h
    include/Levi/Physics2D.h
    include/Levi/Modules.h
    include/Levi/SceneSerializer.h
)

target_include_directories(LeviEngine PUBLIC 
    include
)

# SDL3 target name is usually SDL3::SDL3-static
target_link_libraries(LeviEngine PUBLIC 
    SDL3::SDL3-static 
    SDL3_image::SDL3_image-static 
    flecs_static 
    imgui
    liblua
    sol2
)

# ImGui source for internal use
target_include_directories(LeviEngine PRIVATE third_party/imgui)
