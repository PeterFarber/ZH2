add_executable(HockeyEditorTests
    src/EditorTestsMain.cpp
    src/EditorVersionTests.cpp
    src/EditorSettingsTests.cpp
    src/SelectionTests.cpp
    src/InspectorFieldTests.cpp
    src/UndoRedoTests.cpp
    src/ToolTests.cpp
    src/HockeyPhysicsSetupTests.cpp
    src/SceneWorkflowTests.cpp
    src/ProjectBrowserTests.cpp
)

target_link_libraries(HockeyEditorTests PRIVATE hockey_core hockey_ecs hockey_renderer hockey_editor hockey_physics hockey_gameplay)
set_target_properties(HockeyEditorTests PROPERTIES OUTPUT_NAME "HockeyEditorTests")
