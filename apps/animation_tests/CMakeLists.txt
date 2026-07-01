add_executable(HockeyAnimationTests
    src/AnimationTestsMain.cpp
    src/AnimationLibraryContractTests.cpp
    src/AnimationSettingsTests.cpp
    src/SkeletonTests.cpp
    src/AnimationClipTests.cpp
    src/AnimationSamplerTests.cpp
    src/AnimationBlendTests.cpp
    src/AnimationComponentSerializerTests.cpp
    src/AnimationGraphTests.cpp
)

target_link_libraries(HockeyAnimationTests PRIVATE hockey_core hockey_ecs hockey_assets hockey_animation)
set_target_properties(HockeyAnimationTests PROPERTIES OUTPUT_NAME "HockeyAnimationTests")
