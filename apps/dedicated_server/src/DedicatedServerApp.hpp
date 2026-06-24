#pragma once
#include "Hockey/Core/Config.hpp"
#include "Hockey/Core/HeadlessApplication.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"
#include <cstdint>

namespace Hockey {
class PhysicsSystem;
}

class DedicatedServerApp final : public Hockey::HeadlessApplication {
public:
    using HeadlessApplication::HeadlessApplication;

protected:
    bool OnInit() override;
    void OnShutdown() override;
    void OnFixedUpdate(double fixedDeltaSeconds) override;

private:
    Hockey::Config m_Config;
    Hockey::Scene m_Scene{"Server Scene"};
    Hockey::GameplayWorld m_GameplayWorld;
    Hockey::GameplaySettings m_GameplaySettings;
    Hockey::GameplayTuning m_GameplayTuning;
    Hockey::PhysicsSystem* m_PhysicsSystem = nullptr; // owned by m_Scene
    bool m_GameplayEnabled = false;
    bool m_PhysicsDebug = false;
    uint64_t m_Tick = 0;
    uint64_t m_TicksPerLog = 60;
    uint64_t m_MaxTicks = 0;
};
