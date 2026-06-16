find_package(Jolt CONFIG REQUIRED)

add_library(hockey_physics STATIC)

target_sources(hockey_physics
    PRIVATE
        src/Physics.cpp
        src/PhysicsSettings.cpp
        src/PhysicsLayer.cpp
        src/PhysicsMaterial.cpp
        src/PhysicsMeshProvider.cpp
        src/PhysicsComponents.cpp
        src/PhysicsValidation.cpp
        src/PhysicsShape.cpp
        src/PhysicsDebugDraw.cpp
        src/PhysicsWorld.cpp
        src/PhysicsScene.cpp
        src/PhysicsSystem.cpp
        src/Jolt/JoltCommon.cpp
        src/Jolt/JoltBroadPhaseLayer.cpp
        src/Jolt/JoltTypeConversions.cpp
        src/Jolt/JoltShapeFactory.cpp
        src/Jolt/JoltBodyInterface.cpp
        src/Jolt/JoltContactListener.cpp
        src/Jolt/JoltPhysicsWorld.cpp
        src/Jolt/JoltQueries.cpp
        src/Jolt/JoltDebugRenderer.cpp
)

target_include_directories(hockey_physics
    PUBLIC include
)

# hockey_physics owns Jolt; the Jolt headers/symbols stay private so downstream
# libraries (editor, apps) never inherit a Jolt include/link dependency.
target_link_libraries(hockey_physics
    PUBLIC
        hockey_core
        hockey_ecs
    PRIVATE
        Jolt::Jolt
)

if(MSVC)
    target_compile_options(hockey_physics PRIVATE /W4)
else()
    target_compile_options(hockey_physics PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_physics PRIVATE /WX)
    else()
        target_compile_options(hockey_physics PRIVATE -Werror)
    endif()
endif()
