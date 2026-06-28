add_executable(HockeyGameClientTests
    src/GameClientTestsMain.cpp
    src/ConfigPathTests.cpp
    src/GameplayCameraTests.cpp
    src/GameplayPresentationTests.cpp
    ${CMAKE_SOURCE_DIR}/apps/game_client/src/GameplayCamera.cpp
    ${CMAKE_SOURCE_DIR}/apps/game_client/src/GameplayPresentation.cpp
)

target_include_directories(HockeyGameClientTests PRIVATE
    ${CMAKE_SOURCE_DIR}/apps/game_client/src
)
target_link_libraries(HockeyGameClientTests PRIVATE hockey_core hockey_ecs hockey_gameplay)
set_target_properties(HockeyGameClientTests PROPERTIES OUTPUT_NAME "HockeyGameClientTests")
