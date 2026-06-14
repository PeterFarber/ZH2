#include "Hockey/Physics/PhysicsSystem.hpp"

#include <utility>

#include "Hockey/Core/Log.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneMode.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"

namespace Hockey {

PhysicsSystem::PhysicsSystem(PhysicsSettings settings) : m_Settings(std::move(settings)) {}

void PhysicsSystem::OnStart(Scene& scene) {
    Status status = m_PhysicsScene.Init(scene, m_Settings);
    if (!status) {
        HK_CORE_ERROR("PhysicsSystem failed to initialise: {}", status.error);
        return;
    }
    m_PhysicsScene.OnSimulationStart(scene);
}

void PhysicsSystem::OnStop(Scene& scene) {
    if (!m_PhysicsScene.IsInitialized()) {
        return;
    }
    m_PhysicsScene.OnSimulationStop(scene);
    m_PhysicsScene.Shutdown();
}

void PhysicsSystem::OnFixedUpdate(Scene& scene, float fixedDeltaTime) {
    if (!ShouldSimulate(scene)) {
        return;
    }
    m_PhysicsScene.OnFixedUpdate(scene, fixedDeltaTime);
}

PhysicsWorld& PhysicsSystem::World() {
    return m_PhysicsScene.World();
}

const PhysicsWorld& PhysicsSystem::World() const {
    return m_PhysicsScene.World();
}

const PhysicsSettings& PhysicsSystem::Settings() const {
    return m_Settings;
}

void PhysicsSystem::SetEditorPreviewEnabled(bool enabled) {
    m_EditorPreviewEnabled = enabled;
}

bool PhysicsSystem::IsEditorPreviewEnabled() const {
    return m_EditorPreviewEnabled;
}

bool PhysicsSystem::IsInitialized() const {
    return m_PhysicsScene.IsInitialized();
}

bool PhysicsSystem::ShouldSimulate(const Scene& scene) const {
    if (!m_PhysicsScene.IsInitialized()) {
        return false;
    }
    if (scene.GetMode() == SceneMode::Edit) {
        return m_EditorPreviewEnabled;
    }
    return true;
}

} // namespace Hockey
