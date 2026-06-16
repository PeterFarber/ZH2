#pragma once

#include <string_view>

namespace Hockey {

// How a Scene is being used. Drives future system selection (which systems run,
// whether the simulation is authoritative, etc). Phase 2 only stores the value.
enum class SceneMode {
    Edit,            // map editor editing mode
    Play,            // local / game-client runtime scene
    Server,          // authoritative dedicated server scene
    ClientPrediction // future client prediction / interpolation scene
};

// Stable string tokens used when serializing the mode to scene YAML. Kept in
// sync with the enum; unknown tokens fall back to the supplied default on load.
constexpr std::string_view SceneModeToString(SceneMode mode) {
    switch (mode) {
    case SceneMode::Edit:
        return "Edit";
    case SceneMode::Play:
        return "Play";
    case SceneMode::Server:
        return "Server";
    case SceneMode::ClientPrediction:
        return "ClientPrediction";
    }
    return "Edit";
}

constexpr SceneMode SceneModeFromString(std::string_view text, SceneMode fallback = SceneMode::Edit) {
    if (text == "Edit") {
        return SceneMode::Edit;
    }
    if (text == "Play") {
        return SceneMode::Play;
    }
    if (text == "Server") {
        return SceneMode::Server;
    }
    if (text == "ClientPrediction") {
        return SceneMode::ClientPrediction;
    }
    return fallback;
}

} // namespace Hockey
