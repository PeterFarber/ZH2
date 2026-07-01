#pragma once

#include <cstdint>
#include <string>

#include "Hockey/ECS/Components.hpp"

namespace Hockey {

struct AnimatorComponent {
    uint64_t animationGraphAsset = 0;
    std::string currentState;
    float playbackTimeSeconds = 0.0f;
    float playbackSpeed = 1.0f;
    bool playing = true;
};

struct AnimationParameterComponent {
    float speed = 0.0f;
    float shotChargeRatio = 0.0f;
    bool hasPuck = false;
    bool shooting = false;
    bool stealing = false;
    bool goalieAction = false;
    bool celebrating = false;
};

template <> struct ComponentTraits<AnimatorComponent> {
    static constexpr const char* Name = "AnimatorComponent";
};

template <> struct ComponentTraits<AnimationParameterComponent> {
    static constexpr const char* Name = "AnimationParameterComponent";
};

} // namespace Hockey
