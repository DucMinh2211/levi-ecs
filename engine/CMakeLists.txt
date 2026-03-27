add_library(LeviEngine STATIC
    src/Engine.cpp
    src/AssetManager.cpp
    include/Levi/Engine.h
    include/Levi/AssetManager.h
    include/Levi/Math.h
    include/Levi/Components.h
)

target_include_directories(LeviEngine PUBLIC 
    include
)

# SDL3 target name is usually SDL3::SDL3-static
target_link_libraries(LeviEngine PUBLIC SDL3::SDL3-static SDL3_image::SDL3_image-static flecs_static imgui)

# ImGui source for internal use
target_include_directories(LeviEngine PRIVATE ${imgui_SOURCE_DIR})
