add_executable(HockeyGameplayTests
    src/GameplayTestsMain.cpp
    src/SettingsTuningTests.cpp
    src/InputModelTests.cpp
    src/EventTests.cpp
    src/TeamTypesTests.cpp
    src/GameplayComponentTests.cpp
    src/MatchSetupTests.cpp
    src/GameplayWorldTests.cpp
    src/ResetSystemTests.cpp
    src/PlayerMovementTests.cpp
    src/WaypointMarkerTests.cpp
    src/PuckInteractionTests.cpp
    src/PuckControllerTests.cpp
    src/ShootingTests.cpp
    src/StealTests.cpp
    src/GoalDetectionTests.cpp
    src/OutOfPlayTests.cpp
    src/GameplaySnapshotTests.cpp
    src/MainRinkGameplayTests.cpp
    src/HeadlessServerGameplayTests.cpp
)

target_link_libraries(HockeyGameplayTests PRIVATE hockey_core hockey_ecs hockey_physics hockey_gameplay)
set_target_properties(HockeyGameplayTests PROPERTIES OUTPUT_NAME "HockeyGameplayTests")
