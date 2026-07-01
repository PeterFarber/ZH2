add_executable(HockeyMapEditor
    src/MapEditorMain.cpp
    src/MapEditorApp.cpp
    src/MapEditorApp.hpp
)

target_link_libraries(HockeyMapEditor PRIVATE hockey_core hockey_ecs hockey_renderer hockey_assets hockey_animation hockey_editor hockey_physics hockey_gameplay)
set_target_properties(HockeyMapEditor PROPERTIES OUTPUT_NAME "HockeyMapEditor")

add_custom_command(TARGET HockeyMapEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:HockeyMapEditor>/../../data/config"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/data/config/editor.toml"
        "$<TARGET_FILE_DIR:HockeyMapEditor>/../../data/config/editor.toml"
    COMMENT "Copying editor-managed config to build-local data/config"
)
