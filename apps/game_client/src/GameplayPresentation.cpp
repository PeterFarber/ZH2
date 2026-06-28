#include "GameplayPresentation.hpp"

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/Player/PlayerComponents.hpp"
#include "Hockey/Gameplay/Puck/PuckComponents.hpp"

#include <algorithm>

#include <entt/entt.hpp>
#include <glm/common.hpp>

namespace HockeyClient {
namespace {

float ClampAlpha(float alpha) {
    return std::clamp(alpha, 0.0f, 1.0f);
}

bool ShouldCapture(Hockey::Entity entity) {
    return entity.IsActiveInHierarchy() && entity.HasComponent<Hockey::TransformComponent>() &&
           (entity.HasComponent<Hockey::PlayerComponent>() || entity.HasComponent<Hockey::PuckComponent>());
}

void CaptureEntity(Hockey::Entity entity, GameplayPresentationState::Sample& sample) {
    const glm::vec3 position = entity.GetComponent<Hockey::TransformComponent>().localPosition;
    if (!sample.initialized) {
        sample.previousPosition = position;
        sample.currentPosition = position;
        sample.initialized = true;
        return;
    }
    sample.previousPosition = sample.currentPosition;
    sample.currentPosition = position;
}

void ApplyEntity(Hockey::Entity entity,
                 const GameplayPresentationState::Sample& sample,
                 float alpha,
                 std::vector<ScopedGameplayPresentationTransforms::RestoreEntry>& restore) {
    if (!ShouldCapture(entity) || !sample.initialized) {
        return;
    }

    Hockey::TransformComponent& transform = entity.GetComponent<Hockey::TransformComponent>();
    restore.push_back({entity.GetUUID(), transform.localPosition});
    transform.localPosition = glm::mix(sample.previousPosition, sample.currentPosition, alpha);
}

} // namespace

void GameplayPresentationState::CaptureFixedStep(Hockey::Scene& scene) {
    auto players = scene.Registry().view<Hockey::PlayerComponent, Hockey::TransformComponent>();
    for (const entt::entity handle : players) {
        Hockey::Entity entity(handle, &scene);
        if (ShouldCapture(entity)) {
            CaptureEntity(entity, m_Samples[entity.GetUUID().Value()]);
        }
    }

    auto pucks = scene.Registry().view<Hockey::PuckComponent, Hockey::TransformComponent>();
    for (const entt::entity handle : pucks) {
        Hockey::Entity entity(handle, &scene);
        if (ShouldCapture(entity)) {
            CaptureEntity(entity, m_Samples[entity.GetUUID().Value()]);
        }
    }
}

bool GameplayPresentationState::GetSample(Hockey::UUID entityId, Sample& outSample) const {
    const auto sampleIt = m_Samples.find(entityId.Value());
    if (sampleIt == m_Samples.end()) {
        return false;
    }
    outSample = sampleIt->second;
    return true;
}

void GameplayPresentationState::Reset() {
    m_Samples.clear();
}

ScopedGameplayPresentationTransforms::ScopedGameplayPresentationTransforms(
    Hockey::Scene& scene,
    const GameplayPresentationState& state,
    float alpha,
    const GameplayPresentationSettings& settings)
    : m_Scene(&scene) {
    if (!settings.enabled) {
        return;
    }

    const float clampedAlpha = ClampAlpha(alpha);
    if (settings.interpolatePlayers) {
        auto players = scene.Registry().view<Hockey::PlayerComponent, Hockey::TransformComponent>();
        for (const entt::entity handle : players) {
            Hockey::Entity entity(handle, &scene);
            const auto sampleIt = state.m_Samples.find(entity.GetUUID().Value());
            if (sampleIt != state.m_Samples.end()) {
                ApplyEntity(entity, sampleIt->second, clampedAlpha, m_Restore);
            }
        }
    }

    if (settings.interpolatePuck) {
        auto pucks = scene.Registry().view<Hockey::PuckComponent, Hockey::TransformComponent>();
        for (const entt::entity handle : pucks) {
            Hockey::Entity entity(handle, &scene);
            const auto sampleIt = state.m_Samples.find(entity.GetUUID().Value());
            if (sampleIt != state.m_Samples.end()) {
                ApplyEntity(entity, sampleIt->second, clampedAlpha, m_Restore);
            }
        }
    }
}

ScopedGameplayPresentationTransforms::~ScopedGameplayPresentationTransforms() {
    if (m_Scene == nullptr) {
        return;
    }

    for (const RestoreEntry& entry : m_Restore) {
        Hockey::Entity entity = m_Scene->FindEntityByUUID(entry.entity);
        if (entity.IsValid() && entity.HasComponent<Hockey::TransformComponent>()) {
            entity.GetComponent<Hockey::TransformComponent>().localPosition = entry.localPosition;
        }
    }
}

} // namespace HockeyClient
