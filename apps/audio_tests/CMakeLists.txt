add_executable(HockeyAudioTests
    src/AudioTestsMain.cpp
    src/AudioSettingsTests.cpp
    src/AudioMixerTests.cpp
    src/AudioClipTests.cpp
    src/AudioSystemTests.cpp
    src/AudioComponentTests.cpp
    src/HockeyAudioControllerTests.cpp
    src/HockeyAudioCueSettingsTests.cpp
)

target_link_libraries(HockeyAudioTests PRIVATE hockey_core hockey_ecs hockey_assets hockey_audio)
set_target_properties(HockeyAudioTests PROPERTIES OUTPUT_NAME "HockeyAudioTests")
