#pragma once
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/FixedTimestep.hpp"
#include "Hockey/Core/WindowedApplication.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayInput.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Renderer/Renderer.hpp"

#include <cstdint>

namespace Hockey {
class PhysicsSystem;
}

class GameClientApp final : public Hockey::WindowedApplication {
public:
    using WindowedApplication::WindowedApplication;

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnUpdate(float deltaTime) override;
    void OnEvent(const Hockey::Event& event) override;

private:
    void StepSimulation(float deltaTime);
    Hockey::GameplayInputFrame BuildLocalInput(uint64_t simulationTick);
    void SubmitPhysicsDebugDraw();

    Hockey::Config m_Config;
    Hockey::Scene m_Scene{"Game Scene"};
    Hockey::GameplayWorld m_GameplayWorld;
    Hockey::GameplaySettings m_GameplaySettings;
    Hockey::Renderer m_Renderer;
    Hockey::AssetManager m_AssetManager;
    bool m_RendererReady = false;
    bool m_AssetsReady = false;

    Hockey::PhysicsSystem* m_PhysicsSystem = nullptr; // owned by m_Scene
    Hockey::FixedTimestep m_SimulationTimestep{60.0};
    uint64_t m_LocalInputSequence = 0;
    bool m_LocalGameplayEnabled = false;
    bool m_PhysicsReady = false;
    bool m_PhysicsDebug = false;

    uint32_t m_LastWidth = 0;
    uint32_t m_LastHeight = 0;
};
