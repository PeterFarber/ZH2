add_executable(HockeyMapEditor
    src/MapEditorMain.cpp
    src/MapEditorApp.cpp
    src/MapEditorApp.hpp
)

target_link_libraries(HockeyMapEditor PRIVATE hockey_core hockey_ecs hockey_renderer hockey_assets hockey_editor hockey_physics)
set_target_properties(HockeyMapEditor PROPERTIES OUTPUT_NAME "HockeyMapEditor")
