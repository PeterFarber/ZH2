#include "Hockey/Animation/AnimationEvents.hpp"

#include <algorithm>
#include <cmath>

namespace Hockey {
namespace {

float WrapTime(float timeSeconds, float durationSeconds) {
    float wrapped = std::fmod(timeSeconds, durationSeconds);
    if (wrapped < 0.0f) {
        wrapped += durationSeconds;
    }
    return wrapped;
}

void AppendEventsInRange(const AnimationClip& clip,
                         const std::string& stateName,
                         float startExclusive,
                         float endInclusive,
                         std::vector<AnimationEvent>& outEvents) {
    for (const AnimationEventKey& key : clip.events) {
        if (key.timeSeconds > startExclusive && key.timeSeconds <= endInclusive) {
            outEvents.push_back({clip.id, stateName, key.name, key.timeSeconds});
        }
    }
}

} // namespace

std::vector<AnimationEvent> CollectAnimationEvents(const AnimationClip& clip,
                                                   const std::string& stateName,
                                                   float previousTimeSeconds,
                                                   float currentTimeSeconds,
                                                   bool looping) {
    std::vector<AnimationEvent> events;
    if (clip.events.empty() || clip.durationSeconds <= 0.0f || currentTimeSeconds <= previousTimeSeconds) {
        return events;
    }

    if (!looping) {
        const float start = std::clamp(previousTimeSeconds, 0.0f, clip.durationSeconds);
        const float end = std::clamp(currentTimeSeconds, 0.0f, clip.durationSeconds);
        if (end > start) {
            AppendEventsInRange(clip, stateName, start, end, events);
        }
        return events;
    }

    const float duration = clip.durationSeconds;
    if (currentTimeSeconds - previousTimeSeconds >= duration) {
        AppendEventsInRange(clip, stateName, -0.0f, duration, events);
        return events;
    }

    const float previousWrapped = WrapTime(previousTimeSeconds, duration);
    const float currentWrapped = WrapTime(currentTimeSeconds, duration);
    if (currentWrapped > previousWrapped) {
        AppendEventsInRange(clip, stateName, previousWrapped, currentWrapped, events);
    } else {
        AppendEventsInRange(clip, stateName, previousWrapped, duration, events);
        AppendEventsInRange(clip, stateName, -0.0f, currentWrapped, events);
    }

    return events;
}

} // namespace Hockey
