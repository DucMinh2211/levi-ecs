add_executable(LeviEditor 
    src/main.cpp
    src/ProjectExplorer.cpp
    src/ProjectExplorer.h
    src/SceneHierarchy.cpp
    src/SceneHierarchy.h
    src/Inspector.cpp
    src/Inspector.h
)

target_include_directories(LeviEditor PRIVATE 
    ${CMAKE_SOURCE_DIR}/engine/include
)

target_link_libraries(LeviEditor PRIVATE LeviEngine nfd)

# Link ImGui and GLAD for editor
# target_link_libraries(LeviEditor PRIVATE ImGui GLAD)
