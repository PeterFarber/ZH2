add_executable(HockeyGameClient
    src/GameClientMain.cpp
    src/GameClientApp.cpp
    src/GameClientApp.hpp
)

target_link_libraries(HockeyGameClient PRIVATE hockey_core hockey_ecs hockey_renderer hockey_assets hockey_physics hockey_gameplay)
set_target_properties(HockeyGameClient PROPERTIES OUTPUT_NAME "HockeyGameClient")
