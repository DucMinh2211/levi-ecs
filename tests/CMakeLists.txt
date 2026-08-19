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
