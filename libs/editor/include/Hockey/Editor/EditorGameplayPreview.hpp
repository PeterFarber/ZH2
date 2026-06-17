#pragma once

#include <cstdint>
#include <filesystem>

#include "Hockey/Core/FixedTimestep.hpp"
#include "Hockey/Core/Result.hpp"
#include "Hockey/Gameplay/GameplayInput.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"

namespace Hockey {

class EditorPhysicsPreview;
class Scene;

// Non-destructive gameplay preview layered on the editor physics preview.
// Gameplay can add runtime match/team state, so the authored scene is restored
// from a full serialized snapshot when preview stops or resets.
class EditorGameplayPreview {
public:
    void Configure(const GameplaySettings& settings);

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
    GameplayInputFrame BuildLocalInput(std::uint64_t simulationTick);
    void StepFixed(Scene& scene, EditorPhysicsPreview& physicsPreview, float fixedDeltaSeconds);

    GameplaySettings m_Settings{};
    GameplayWorld m_World;
    FixedTimestep m_Timestep{60.0};
    std::filesystem::path m_SnapshotPath;
    std::uint64_t m_LocalInputSequence = 0;
    std::uint64_t m_Tick = 0;
    bool m_Active = false;
    bool m_Running = false;
    bool m_StartedPhysicsPreview = false;
};

} // namespace Hockey
