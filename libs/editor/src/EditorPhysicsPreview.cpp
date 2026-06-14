#include "Hockey/Editor/EditorPhysicsPreview.hpp"

#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsEvents.hpp"

namespace Hockey {

void EditorPhysicsPreview::Configure(const PhysicsSettings& settings) {
    m_Settings = settings;
    if (m_Settings.fixedDeltaSeconds > 0.0f) {
        m_Timestep.SetTickRate(1.0 / static_cast<double>(m_Settings.fixedDeltaSeconds));
    }
}

void EditorPhysicsPreview::Start(Scene& scene) {
    if (m_Active || !Physics::IsInitialized()) {
        return;
    }
    SnapshotTransforms(scene);

    if (!m_PhysicsScene.Init(scene, m_Settings)) {
        m_SavedTransforms.clear();
        return;
    }
    m_PhysicsScene.OnSimulationStart(scene);
    m_Timestep.Reset();
    m_Active = true;
    m_Running = false;
}

void EditorPhysicsPreview::Stop(Scene& scene) {
    if (!m_Active) {
        return;
    }
    m_PhysicsScene.OnSimulationStop(scene);
    m_PhysicsScene.Shutdown();
    RestoreTransforms(scene);
    m_SavedTransforms.clear();
    m_Active = false;
    m_Running = false;
}

void EditorPhysicsPreview::Play() {
    if (m_Active) {
        m_Running = true;
    }
}

void EditorPhysicsPreview::Pause() {
    m_Running = false;
}

void EditorPhysicsPreview::StepOnce(Scene& scene) {
    if (!m_Active) {
        return;
    }
    m_Running = false;
    m_PhysicsScene.OnFixedUpdate(scene, m_Settings.fixedDeltaSeconds);
}

void EditorPhysicsPreview::Reset(Scene& scene) {
    if (!m_Active) {
        return;
    }
    m_Running = false;
    RestoreTransforms(scene);
    // Rebuild the preview world from the restored transforms.
    m_PhysicsScene.OnSimulationStart(scene);
    m_Timestep.Reset();
}

void EditorPhysicsPreview::Update(Scene& scene, float deltaTime) {
    if (!m_Active || !m_Running) {
        return;
    }
    const int steps = m_Timestep.Accumulate(static_cast<double>(deltaTime));
    const float fixedDelta = static_cast<float>(m_Timestep.GetFixedDeltaSeconds());
    for (int i = 0; i < steps; ++i) {
        m_PhysicsScene.OnFixedUpdate(scene, fixedDelta);
        m_Timestep.AdvanceTick();
    }

    // Capture contact points for the overlay, then keep event queues bounded.
    if (steps > 0) {
        m_ContactPoints.clear();
        for (const PhysicsContactEvent& contact : m_PhysicsScene.World().DrainContactEvents()) {
            m_ContactPoints.push_back(contact.contactPoint);
        }
        m_PhysicsScene.World().DrainTriggerEvents();
    }
}

void EditorPhysicsPreview::SnapshotTransforms(Scene& scene) {
    m_SavedTransforms.clear();
    for (Entity entity : scene.GetAllEntities()) {
        if (entity.HasComponent<TransformComponent>()) {
            m_SavedTransforms[entity.GetUUID()] = entity.GetComponent<TransformComponent>();
        }
    }
}

void EditorPhysicsPreview::RestoreTransforms(Scene& scene) {
    for (const auto& [id, transform] : m_SavedTransforms) {
        Entity entity = scene.FindEntityByUUID(id);
        if (entity && entity.HasComponent<TransformComponent>()) {
            entity.GetComponent<TransformComponent>() = transform;
        }
    }
}

} // namespace Hockey
