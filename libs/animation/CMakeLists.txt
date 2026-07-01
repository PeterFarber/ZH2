# hockey_animation: CPU-side animation data, sampling, state selection, and
# presentation-facing animation runtime. It must not depend on renderer/editor.

find_package(glm CONFIG REQUIRED)

add_library(hockey_animation STATIC)

target_sources(hockey_animation
    PRIVATE
        src/Animation.cpp
        src/AnimationSettings.cpp
        src/Skeleton.cpp
        src/AnimationClip.cpp
        src/AnimationPose.cpp
        src/AnimationSampler.cpp
        src/AnimationBlend.cpp
        src/AnimationGraph.cpp
        src/AnimationStateMachine.cpp
        src/AnimationEvents.cpp
        src/AnimationSystem.cpp
        src/HockeyAnimationController.cpp
        src/AnimationComponents.cpp
        src/AnimationSerializer.cpp
)

target_include_directories(hockey_animation
    PUBLIC include
)

target_link_libraries(hockey_animation
    PUBLIC
        hockey_core
        hockey_ecs
        hockey_assets
        glm::glm
)

if(MSVC)
    target_compile_options(hockey_animation PRIVATE /W4)
else()
    target_compile_options(hockey_animation PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_animation PRIVATE /WX)
    else()
        target_compile_options(hockey_animation PRIVATE -Werror)
    endif()
endif()
