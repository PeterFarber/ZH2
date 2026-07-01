#pragma once

#include "Hockey/Animation/AnimationClip.hpp"
#include "Hockey/Assets/AssetID.hpp"

#include <string>
#include <vector>

namespace Hockey {

struct AnimationEvent {
    AssetID clipAsset;
    std::string stateName;
    std::string name;
    float timeSeconds = 0.0f;
};

std::vector<AnimationEvent> CollectAnimationEvents(const AnimationClip& clip,
                                                   const std::string& stateName,
                                                   float previousTimeSeconds,
                                                   float currentTimeSeconds,
                                                   bool looping);

} // namespace Hockey
