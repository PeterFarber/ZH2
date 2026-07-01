#pragma once

#include <cstdint>

namespace Hockey {

struct AnimationSettings {
    bool enabled = true;
    bool enableAnimationEvents = true;
    bool enableRootMotion = false;
    float defaultBlendTimeSeconds = 0.15f;
    uint32_t maxBones = 128;
    bool debugDrawSkeletons = false;
    bool logAnimationEvents = false;
};

} // namespace Hockey
