add_executable(HockeyECSTests
    src/ECSTestsMain.cpp
    src/EntityTests.cpp
    src/ComponentTests.cpp
    src/HierarchyTests.cpp
    src/TransformTests.cpp
    src/SceneSerializationTests.cpp
    src/SceneValidationTests.cpp
    src/PrefabTests.cpp
    src/ComponentMetadataTests.cpp
    src/SceneLifecycleTests.cpp
    src/SceneEventTests.cpp
    src/RenderComponentTests.cpp
)

target_link_libraries(HockeyECSTests PRIVATE hockey_core hockey_ecs)
set_target_properties(HockeyECSTests PROPERTIES OUTPUT_NAME "HockeyECSTests")
