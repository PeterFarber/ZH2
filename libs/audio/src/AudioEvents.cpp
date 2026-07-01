#include "Hockey/Audio/AudioEvents.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Core/Config.hpp"

namespace Hockey {
namespace {

const std::array<AudioCueConfigBinding, kHockeyAudioCueConfigBindingCount> kHockeyAudioCueBindings{{
    {AudioCueId::Countdown, "audio_cues.countdown", "Countdown", "data/raw/audio/Countdown.mp3"},
    {AudioCueId::Shot, "audio_cues.shot", "Shot", "data/raw/audio/Shot.mp3"},
    {AudioCueId::OneTimer, "audio_cues.one_timer", "One Timer", "data/raw/audio/OneTimer.mp3"},
    {AudioCueId::PuckCollision, "audio_cues.puck_collision", "Puck Collision",
     "data/raw/audio/PuckCollision.mp3"},
    {AudioCueId::PuckMetalCollision, "audio_cues.puck_metal_collision", "Puck Metal Collision",
     "data/raw/audio/PuckMetalCollision.mp3"},
    {AudioCueId::GoalieShield, "audio_cues.goalie_shield", "Goalie Shield", "data/raw/audio/Shield.mp3"},
    {AudioCueId::PlayerBoost, "audio_cues.player_boost", "Player Boost", "data/raw/audio/Zoom.mp3"},
    {AudioCueId::UIClick, "audio_cues.ui_click", "UI Click", "data/raw/audio/Countdown.mp3"},
    {AudioCueId::UIBack, "audio_cues.ui_back", "UI Back", "data/raw/audio/Countdown.mp3"},
    {AudioCueId::UIConfirm, "audio_cues.ui_confirm", "UI Confirm", "data/raw/audio/Countdown.mp3"},
}};

void SetClipForCue(AudioCueMap& map, AudioCueId cue, AssetID clip) {
    switch (cue) {
    case AudioCueId::Countdown:
        map.countdown = clip;
        break;
    case AudioCueId::Shot:
        map.shot = clip;
        break;
    case AudioCueId::OneTimer:
        map.oneTimer = clip;
        break;
    case AudioCueId::PuckCollision:
        map.puckCollision = clip;
        break;
    case AudioCueId::PuckMetalCollision:
        map.puckMetalCollision = clip;
        break;
    case AudioCueId::GoalieShield:
        map.goalieShield = clip;
        break;
    case AudioCueId::PlayerBoost:
        map.playerBoost = clip;
        break;
    case AudioCueId::UIClick:
        map.uiClick = clip;
        break;
    case AudioCueId::UIBack:
        map.uiBack = clip;
        break;
    case AudioCueId::UIConfirm:
        map.uiConfirm = clip;
        break;
    }
}

} // namespace

AudioCueMap MakeDefaultHockeyAudioCueMap() {
    return {};
}

AssetID ClipForCue(const AudioCueMap& map, AudioCueId cue) {
    switch (cue) {
    case AudioCueId::Countdown:
        return map.countdown;
    case AudioCueId::Shot:
        return map.shot;
    case AudioCueId::OneTimer:
        return map.oneTimer;
    case AudioCueId::PuckCollision:
        return map.puckCollision;
    case AudioCueId::PuckMetalCollision:
        return map.puckMetalCollision;
    case AudioCueId::GoalieShield:
        return map.goalieShield;
    case AudioCueId::PlayerBoost:
        return map.playerBoost;
    case AudioCueId::UIClick:
        return map.uiClick;
    case AudioCueId::UIBack:
        return map.uiBack;
    case AudioCueId::UIConfirm:
        return map.uiConfirm;
    }
    return {};
}

AudioBusType BusForCue(AudioCueId cue) {
    switch (cue) {
    case AudioCueId::Countdown:
    case AudioCueId::UIClick:
    case AudioCueId::UIBack:
    case AudioCueId::UIConfirm:
        return AudioBusType::UI;
    case AudioCueId::PuckCollision:
    case AudioCueId::PuckMetalCollision:
    case AudioCueId::Shot:
    case AudioCueId::OneTimer:
    case AudioCueId::GoalieShield:
    case AudioCueId::PlayerBoost:
        return AudioBusType::SFX;
    }
    return AudioBusType::SFX;
}

const std::array<AudioCueConfigBinding, kHockeyAudioCueConfigBindingCount>& HockeyAudioCueConfigBindings() {
    return kHockeyAudioCueBindings;
}

std::string HockeyAudioCueRawPath(const Config& config, const AudioCueConfigBinding& binding) {
    return config.GetString(binding.key, binding.defaultRawPath);
}

void SaveDefaultHockeyAudioCuePaths(Config& config) {
    for (const AudioCueConfigBinding& binding : kHockeyAudioCueBindings) {
        config.SetString(binding.key, binding.defaultRawPath);
    }
}

AudioCueMap ResolveHockeyAudioCueMap(const Config& config, const AssetManager* assetManager) {
    AudioCueMap map = MakeDefaultHockeyAudioCueMap();
    if (assetManager == nullptr) {
        return map;
    }

    for (const AudioCueConfigBinding& binding : kHockeyAudioCueBindings) {
        const std::string rawPath = HockeyAudioCueRawPath(config, binding);
        const AssetMetadata* metadata = assetManager->Database().FindByRawPath(rawPath);
        if (metadata != nullptr && metadata->type == AssetType::Audio) {
            SetClipForCue(map, binding.cue, metadata->id);
        }
    }
    return map;
}

} // namespace Hockey
