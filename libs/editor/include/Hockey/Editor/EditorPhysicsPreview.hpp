#pragma once

#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/FixedTimestep.hpp"
#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/Physics/PhysicsEvents.hpp"
#include "Hockey/Physics/PhysicsScene.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"

namespace Hockey {

class Scene;

// ---------------------------------------------------------------------------
// Internal non-destructive physics backend for editor playtest.
//
// The editor no longer exposes a standalone physics preview UI. Gameplay
// playtest starts this backend, snapshots authored transforms, steps physics,
// and restores the snapshot when play mode stops.
// ---------------------------------------------------------------------------
class EditorPhysicsPreview {
public:
    void Configure(const PhysicsSettings& settings);

    bool IsActive() const {
        return m_Active;
    }
    bool IsRunning() const {
        return m_Active && m_Running;
    }

    // Snapshots transforms and builds the preview world. No-op if already active.
    void Start(Scene& scene);
    // Restores the snapshot and tears the preview world down. No-op if inactive.
    void Stop(Scene& scene);

    void Play();
    void Pause();

    // Advances exactly one fixed step (auto-pauses continuous play).
    void StepOnce(Scene& scene);

    // Advances one fixed step for an active internal preview backend.
    void AdvanceFixed(Scene& scene, float fixedDeltaSeconds);

    // Restores transforms and rebuilds bodies without leaving preview.
    void Reset(Scene& scene);

    // Per-frame tick: advances the world while running.
    void Update(Scene& scene, float deltaTime);

    PhysicsScene& World() {
        return m_PhysicsScene;
    }

    // World-space contact points captured during the most recent stepped frame.
    const std::vector<glm::vec3>& ContactPoints() const {
        return m_ContactPoints;
    }
    std::vector<PhysicsContactEvent> DrainContactEvents();

private:
    void CaptureContactPoints();
    void SnapshotTransforms(Scene& scene);
    void RestoreTransforms(Scene& scene);

    PhysicsSettings m_Settings{};
    PhysicsScene m_PhysicsScene;
    FixedTimestep m_Timestep{60.0};
    bool m_Active = false;
    bool m_Running = false;
    std::unordered_map<UUID, TransformComponent> m_SavedTransforms;
    std::vector<glm::vec3> m_ContactPoints;
    std::vector<PhysicsContactEvent> m_PendingContactEvents;
};

} // namespace Hockey
