#include "Hockey/Audio/AudioBus.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace Hockey {

namespace {

std::string Lower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

} // namespace

const char* AudioBusTypeToString(AudioBusType bus) {
    switch (bus) {
    case AudioBusType::Master:
        return "Master";
    case AudioBusType::Music:
        return "Music";
    case AudioBusType::SFX:
        return "SFX";
    case AudioBusType::Ambience:
        return "Ambience";
    case AudioBusType::UI:
        return "UI";
    case AudioBusType::Crowd:
        return "Crowd";
    default:
        return "Master";
    }
}

bool AudioBusTypeFromString(std::string_view text, AudioBusType& out) {
    const std::string value = Lower(text);
    if (value == "master") {
        out = AudioBusType::Master;
        return true;
    }
    if (value == "music") {
        out = AudioBusType::Music;
        return true;
    }
    if (value == "sfx") {
        out = AudioBusType::SFX;
        return true;
    }
    if (value == "ambience") {
        out = AudioBusType::Ambience;
        return true;
    }
    if (value == "ui") {
        out = AudioBusType::UI;
        return true;
    }
    if (value == "crowd") {
        out = AudioBusType::Crowd;
        return true;
    }
    return false;
}

} // namespace Hockey
