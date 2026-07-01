add_executable(HockeyGameClient
    src/GameClientMain.cpp
    src/GameClientApp.cpp
    src/GameClientApp.hpp
    src/GameplayCamera.cpp
    src/GameplayCamera.hpp
    src/GameplayPresentation.cpp
    src/GameplayPresentation.hpp
)

target_include_directories(HockeyGameClient PRIVATE src)
target_link_libraries(HockeyGameClient PRIVATE hockey_core hockey_ecs hockey_renderer hockey_assets hockey_animation hockey_audio hockey_physics hockey_gameplay hockey_ui)
set_target_properties(HockeyGameClient PROPERTIES OUTPUT_NAME "HockeyGameClient")
