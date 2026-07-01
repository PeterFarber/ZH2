# hockey_audio: runtime audio playback, mixer state, audio components, and
# hockey presentation event mapping. It depends on core, ECS, and assets only.

find_package(glm CONFIG REQUIRED)
find_path(MINIAUDIO_INCLUDE_DIR miniaudio.h REQUIRED)

add_library(hockey_audio STATIC)

target_sources(hockey_audio
    PRIVATE
        src/AudioSettings.cpp
        src/AudioBus.cpp
        src/AudioMixer.cpp
        src/AudioClip.cpp
        src/AudioDevice.cpp
        src/AudioSystem.cpp
        src/AudioComponents.cpp
        src/AudioEvents.cpp
        src/HockeyAudioController.cpp
        src/MiniaudioImpl.cpp
)

target_include_directories(hockey_audio
    PUBLIC include
    PRIVATE ${MINIAUDIO_INCLUDE_DIR}
)

target_link_libraries(hockey_audio
    PUBLIC
        hockey_core
        hockey_ecs
        hockey_assets
        glm::glm
)

if(MSVC)
    target_compile_options(hockey_audio PRIVATE /W4)
else()
    target_compile_options(hockey_audio PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_audio PRIVATE /WX)
    else()
        target_compile_options(hockey_audio PRIVATE -Werror)
    endif()
endif()
