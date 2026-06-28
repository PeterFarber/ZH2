find_package(yaml-cpp CONFIG REQUIRED)

add_library(hockey_gameplay STATIC)

target_sources(hockey_gameplay
    PRIVATE
        src/Gameplay.cpp
        src/GameplaySettings.cpp
        src/GameplayTypes.cpp
        src/GameplayComponents.cpp
        src/GameplayInput.cpp
        src/GameplayEvents.cpp
        src/Match/FaceoffSystem.cpp
        src/Match/MatchClock.cpp
        src/Match/MatchSystem.cpp
        src/Match/ResetSystem.cpp
        src/Player/PlayerFacing.cpp
        src/Player/PlayerMovement.cpp
        src/Puck/PuckController.cpp
        src/Puck/PuckPossession.cpp
        src/Rink/GoalDetection.cpp
        src/Rink/OutOfPlaySystem.cpp
        src/Match/ScoreSystem.cpp
        src/Simulation/GameplaySnapshot.cpp
        src/Simulation/GameplayWorld.cpp
        src/Stick/ShootingSystem.cpp
        src/Stick/StickAttachment.cpp
        src/Stick/StealSystem.cpp
        src/Stick/StickHandling.cpp
        src/Teams/TeamTypes.cpp
        src/Tuning/GameplayTuning.cpp
        src/Tuning/TuningSerializer.cpp
        src/Waypoint/WaypointMarkerSystem.cpp
        src/Validation/GameplayValidation.cpp
)

target_include_directories(hockey_gameplay
    PUBLIC include
)

target_link_libraries(hockey_gameplay
    PUBLIC
        hockey_core
        hockey_ecs
        hockey_physics
    PRIVATE
        yaml-cpp::yaml-cpp
)

if(MSVC)
    target_compile_options(hockey_gameplay PRIVATE /W4)
else()
    target_compile_options(hockey_gameplay PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_gameplay PRIVATE /WX)
    else()
        target_compile_options(hockey_gameplay PRIVATE -Werror)
    endif()
endif()
