add_executable(LeviLifecycleTest
    LuaScriptManagerLifecycleTest.cpp
)

target_link_libraries(LeviLifecycleTest PRIVATE LeviEngine)
target_compile_definitions(LeviLifecycleTest PRIVATE
    LEVI_TEST_PROJECT_DIR="${CMAKE_SOURCE_DIR}/projects/demo-project"
)

add_test(NAME lua_script_manager_lifecycle COMMAND LeviLifecycleTest)

add_executable(UndoRedoManagerTest
    UndoRedoManagerTest.cpp
    ${CMAKE_SOURCE_DIR}/editor/src/UndoRedoManager.cpp
)
target_include_directories(UndoRedoManagerTest PRIVATE ${CMAKE_SOURCE_DIR}/editor/src)
add_test(NAME undo_redo_manager COMMAND UndoRedoManagerTest)

add_executable(DeleteEntityCommandTest
    DeleteEntityCommandTest.cpp
    ${CMAKE_SOURCE_DIR}/editor/src/DeleteEntityCommand.cpp
    ${CMAKE_SOURCE_DIR}/editor/src/UndoRedoManager.cpp
)
target_include_directories(DeleteEntityCommandTest PRIVATE
    ${CMAKE_SOURCE_DIR}/editor/src
    ${CMAKE_SOURCE_DIR}/engine/include
)
target_link_libraries(DeleteEntityCommandTest PRIVATE LeviEngine)
add_test(NAME delete_entity_command COMMAND DeleteEntityCommandTest)

add_executable(SceneSerializerTest SceneSerializerTest.cpp)
target_link_libraries(SceneSerializerTest PRIVATE LeviEngine)
add_test(NAME scene_serializer COMMAND SceneSerializerTest)

add_executable(Physics2DTest Physics2DTest.cpp)
target_link_libraries(Physics2DTest PRIVATE LeviEngine)
add_test(NAME physics_2d COMMAND Physics2DTest)

add_executable(Camera2DTest Camera2DTest.cpp)
target_link_libraries(Camera2DTest PRIVATE LeviEngine)
add_test(NAME camera_2d COMMAND Camera2DTest)
