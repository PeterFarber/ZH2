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
        src/Teams/TeamTypes.cpp
        src/Tuning/GameplayTuning.cpp
        src/Tuning/TuningSerializer.cpp
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
