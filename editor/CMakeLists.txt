add_executable(LeviEditor 
    src/main.cpp
    src/ProjectExplorer.cpp
    src/ProjectExplorer.h
)

target_include_directories(LeviEditor PRIVATE 
    ${CMAKE_SOURCE_DIR}/engine/include
)

target_link_libraries(LeviEditor PRIVATE LeviEngine)

# Link ImGui and GLAD for editor
# target_link_libraries(LeviEditor PRIVATE ImGui GLAD)
