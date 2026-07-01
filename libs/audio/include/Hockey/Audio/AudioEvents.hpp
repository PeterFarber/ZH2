#pragma once

#include <array>
#include <optional>
#include <string>

#include <glm/vec3.hpp>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Audio/AudioBus.hpp"

namespace Hockey {

class AssetManager;
class Config;

enum class AudioCueId {
    Countdown,
    Shot,
    OneTimer,
    PuckCollision,
    PuckMetalCollision,
    GoalieShield,
    PlayerBoost,
    UIClick,
    UIBack,
    UIConfirm,
};

struct AudioCue {
    AssetID clip;
    AudioBusType bus = AudioBusType::SFX;
    float volume = 1.0f;
    std::optional<glm::vec3> position;
};

class IAudioCueSink {
public:
    virtual ~IAudioCueSink() = default;
    virtual void PlayCue(const AudioCue& cue) = 0;
};

struct AudioCueMap {
    AssetID countdown;
    AssetID shot;
    AssetID oneTimer;
    AssetID puckCollision;
    AssetID puckMetalCollision;
    AssetID goalieShield;
    AssetID playerBoost;
    AssetID uiClick;
    AssetID uiBack;
    AssetID uiConfirm;
};

struct AudioCueConfigBinding {
    AudioCueId cue;
    const char* key;
    const char* displayName;
    const char* defaultRawPath;
};

inline constexpr std::size_t kHockeyAudioCueConfigBindingCount = 10;

AudioCueMap MakeDefaultHockeyAudioCueMap();
AssetID ClipForCue(const AudioCueMap& map, AudioCueId cue);
AudioBusType BusForCue(AudioCueId cue);
const std::array<AudioCueConfigBinding, kHockeyAudioCueConfigBindingCount>& HockeyAudioCueConfigBindings();
std::string HockeyAudioCueRawPath(const Config& config, const AudioCueConfigBinding& binding);
void SaveDefaultHockeyAudioCuePaths(Config& config);
AudioCueMap ResolveHockeyAudioCueMap(const Config& config, const AssetManager* assetManager);

} // namespace Hockey
