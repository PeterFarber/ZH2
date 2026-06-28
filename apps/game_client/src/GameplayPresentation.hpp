#pragma once

#include "Hockey/Core/UUID.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <glm/vec3.hpp>

namespace Hockey {
class Scene;
}

namespace HockeyClient {

struct GameplayPresentationSettings {
    bool enabled = true;
    bool interpolatePlayers = true;
    bool interpolatePuck = true;
};

class GameplayPresentationState {
public:
    struct Sample {
        bool initialized = false;
        glm::vec3 previousPosition{0.0f};
        glm::vec3 currentPosition{0.0f};
    };

    void CaptureFixedStep(Hockey::Scene& scene);
    bool GetSample(Hockey::UUID entityId, Sample& outSample) const;
    void Reset();

private:
    friend class ScopedGameplayPresentationTransforms;

    std::unordered_map<std::uint64_t, Sample> m_Samples;
};

class ScopedGameplayPresentationTransforms {
public:
    struct RestoreEntry {
        Hockey::UUID entity;
        glm::vec3 localPosition{0.0f};
    };

    ScopedGameplayPresentationTransforms(Hockey::Scene& scene,
                                         const GameplayPresentationState& state,
                                         float alpha,
                                         const GameplayPresentationSettings& settings);
    ~ScopedGameplayPresentationTransforms();

    ScopedGameplayPresentationTransforms(const ScopedGameplayPresentationTransforms&) = delete;
    ScopedGameplayPresentationTransforms& operator=(const ScopedGameplayPresentationTransforms&) = delete;

private:
    Hockey::Scene* m_Scene = nullptr;
    std::vector<RestoreEntry> m_Restore;
};

} // namespace HockeyClient
