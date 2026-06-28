#pragma once

#include <cstdint>
#include <filesystem>

#include <glm/glm.hpp>

#include "Hockey/Core/FixedTimestep.hpp"
#include "Hockey/Core/Result.hpp"
#include "Hockey/Gameplay/GameplayInput.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

class EditorPhysicsPreview;
class Scene;

// Non-destructive gameplay preview layered on the editor physics preview.
// Gameplay can add runtime match/team state, so the authored scene is restored
// from a full serialized snapshot when preview stops or resets.
class EditorGameplayPreview {
public:
    void Configure(const GameplaySettings& settings);
    void Configure(const GameplaySettings& settings, const GameplayTuning& tuning);
    void SetTuning(const GameplayTuning& tuning);

    bool IsActive() const {
        return m_Active;
    }
    bool IsRunning() const {
        return m_Active && m_Running;
    }

    Status Start(Scene& scene, EditorPhysicsPreview& physicsPreview);
    void Stop(Scene& scene, EditorPhysicsPreview& physicsPreview);

    void Play();
    void Pause();
    void StepOnce(Scene& scene, EditorPhysicsPreview& physicsPreview);
    void Reset(Scene& scene, EditorPhysicsPreview& physicsPreview);
    void Update(Scene& scene, EditorPhysicsPreview& physicsPreview, float deltaTime);
    void SetMoveTarget(const glm::vec3& target);
    void SetAimTarget(const glm::vec3& target);
    void SetInputEnabled(bool enabled) {
        m_InputEnabled = enabled;
    }

    void SetDebugDrawEnabled(bool enabled) {
        m_Settings.debugDrawGameplay = enabled;
    }
    bool DebugDrawEnabled() const {
        return m_Settings.debugDrawGameplay;
    }

private:
    Status SaveAuthoringSnapshot(Scene& scene);
    Status RestoreAuthoringSnapshot(Scene& scene);
    void ClearSnapshot();
    GameplayInputFrame BuildLocalInput(Scene& scene, std::uint64_t simulationTick);
    void StepFixed(Scene& scene, EditorPhysicsPreview& physicsPreview, float fixedDeltaSeconds);

    GameplaySettings m_Settings{};
    GameplayTuning m_Tuning{};
    GameplayWorld m_World;
    FixedTimestep m_Timestep{60.0};
    std::filesystem::path m_SnapshotPath;
    std::uint64_t m_LocalInputSequence = 0;
    std::uint64_t m_Tick = 0;
    glm::vec3 m_MoveTarget{0.0f};
    glm::vec3 m_AimTarget{0.0f};
    glm::vec3 m_FollowCameraPosition{0.0f};
    bool m_Active = false;
    bool m_Running = false;
    bool m_StartedPhysicsPreview = false;
    bool m_HasMoveTarget = false;
    bool m_HasAimTarget = false;
    bool m_InputEnabled = false;
    bool m_FollowCameraInitialized = false;
};

} // namespace Hockey
