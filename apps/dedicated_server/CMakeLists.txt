add_executable(HockeyDedicatedServer
    src/DedicatedServerMain.cpp
    src/DedicatedServerApp.cpp
    src/DedicatedServerApp.hpp
)

target_link_libraries(HockeyDedicatedServer PRIVATE hockey_core hockey_ecs hockey_physics hockey_gameplay)
set_target_properties(HockeyDedicatedServer PROPERTIES OUTPUT_NAME "HockeyDedicatedServer")

add_custom_command(TARGET HockeyDedicatedServer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:HockeyDedicatedServer>/../../data/config"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/data/config/server.toml"
        "$<TARGET_FILE_DIR:HockeyDedicatedServer>/../../data/config/server.toml"
    COMMENT "Copying server config to build-local data/config"
)
