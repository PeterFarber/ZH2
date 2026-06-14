#pragma once

namespace Hockey {

// How a Scene is being used. Drives future system selection (which systems run,
// whether the simulation is authoritative, etc). Phase 2 only stores the value.
enum class SceneMode {
    Edit,            // map editor editing mode
    Play,            // local / game-client runtime scene
    Server,          // authoritative dedicated server scene
    ClientPrediction // future client prediction / interpolation scene
};

} // namespace Hockey
