#pragma once

#include "Hockey/ECS/System.hpp"
#include "Hockey/Physics/PhysicsScene.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"

namespace Hockey {

class Scene;
class PhysicsWorld;

// ---------------------------------------------------------------------------
// Scene-attached system that runs a PhysicsScene through the Phase 2 scene
// lifecycle. Add it to a Scene when physics should run:
//
//   scene.AddSystem(std::make_unique<PhysicsSystem>(settings));
//
// Mode behaviour (see SceneMode):
//   Play / Server      -> simulates every fixed update
//   Edit               -> bodies exist for preview/debug, but the simulation
//                          only advances when preview is explicitly enabled
//   ClientPrediction   -> simulates (prediction wiring lands in a later phase)
// ---------------------------------------------------------------------------
class PhysicsSystem final : public System {
public:
    explicit PhysicsSystem(PhysicsSettings settings);

    void OnStart(Scene& scene) override;
    void OnStop(Scene& scene) override;
    void OnFixedUpdate(Scene& scene, float fixedDeltaTime) override;

    PhysicsWorld& World();
    const PhysicsWorld& World() const;

    const PhysicsSettings& Settings() const;

    // Editor preview toggle for Edit-mode scenes. Ignored in other modes.
    void SetEditorPreviewEnabled(bool enabled);
    bool IsEditorPreviewEnabled() const;

    bool IsInitialized() const;

private:
    bool ShouldSimulate(const Scene& scene) const;

    PhysicsSettings m_Settings;
    PhysicsScene m_PhysicsScene;
    bool m_EditorPreviewEnabled = false;
};

} // namespace Hockey
