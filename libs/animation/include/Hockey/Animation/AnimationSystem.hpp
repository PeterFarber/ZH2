#pragma once

#include "Hockey/Animation/AnimationEvents.hpp"
#include "Hockey/Animation/AnimationGraph.hpp"
#include "Hockey/Animation/AnimationSettings.hpp"
#include "Hockey/Core/Result.hpp"

#include <unordered_map>
#include <vector>

namespace Hockey {

class AssetManager;
class Scene;

struct AnimationSystemStats {
    size_t updatedPaletteCount = 0;
    size_t missingGraphCount = 0;
    size_t missingSkeletonCount = 0;
    size_t missingClipCount = 0;
    size_t invalidSkeletonCount = 0;
    size_t invalidClipCount = 0;
    size_t emittedEventCount = 0;
};

class AnimationSystem {
public:
    explicit AnimationSystem(AnimationSettings settings = {});

    const AnimationSettings& GetSettings() const;
    void SetSettings(AnimationSettings settings);

    Status RegisterGraph(AnimationGraph graph);
    void ClearGraphs();

    Status Update(Scene& scene, AssetManager& assetManager, float dtSeconds);

    const AnimationSystemStats& GetStats() const;
    std::vector<AnimationEvent> DrainEvents();

private:
    const AnimationGraph* FindGraph(AssetID id) const;

    AnimationSettings m_Settings;
    AnimationSystemStats m_Stats;
    std::unordered_map<AssetID, AnimationGraph> m_Graphs;
    std::vector<AnimationEvent> m_Events;
};

} // namespace Hockey
