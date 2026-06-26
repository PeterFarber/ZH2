add_executable(HockeyGameClient
    src/GameClientMain.cpp
    src/GameClientApp.cpp
    src/GameClientApp.hpp
)

target_link_libraries(HockeyGameClient PRIVATE hockey_core hockey_ecs hockey_renderer hockey_assets hockey_physics hockey_gameplay hockey_ui)
set_target_properties(HockeyGameClient PROPERTIES OUTPUT_NAME "HockeyGameClient")

add_custom_command(TARGET HockeyGameClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:HockeyGameClient>/../../data/config"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/data/config/client.toml"
        "$<TARGET_FILE_DIR:HockeyGameClient>/../../data/config/client.toml"
    COMMENT "Copying client config to build-local data/config"
)
