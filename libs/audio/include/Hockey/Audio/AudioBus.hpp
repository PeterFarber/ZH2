#pragma once

#include <string_view>

namespace Hockey {

enum class AudioBusType {
    Master,
    Music,
    SFX,
    Ambience,
    UI,
    Crowd,
};

const char* AudioBusTypeToString(AudioBusType bus);
bool AudioBusTypeFromString(std::string_view text, AudioBusType& out);

} // namespace Hockey
