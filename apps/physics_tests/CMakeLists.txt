add_executable(HockeyPhysicsTests
    src/PhysicsTestsMain.cpp
    src/PhysicsInitTests.cpp
    src/CollisionLayerTests.cpp
    src/PhysicsMaterialTests.cpp
    src/PhysicsComponentTests.cpp
    src/ColliderTests.cpp
    src/PhysicsWorldTests.cpp
    src/RigidBodyTests.cpp
    src/SceneSyncTests.cpp
    src/ContactEventTests.cpp
    src/TriggerTests.cpp
    src/QueryTests.cpp
    src/DebugDrawTests.cpp
    src/HeadlessServerPhysicsTests.cpp
)

target_link_libraries(HockeyPhysicsTests PRIVATE hockey_core hockey_ecs hockey_physics)
set_target_properties(HockeyPhysicsTests PROPERTIES OUTPUT_NAME "HockeyPhysicsTests")
