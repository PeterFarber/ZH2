#pragma once

#include <cstdint>
#include <vector>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Gameplay/GameplayInput.hpp"
#include "Hockey/Gameplay/GameplaySettings.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

class PhysicsWorld;
class Scene;

class GameplayWorld {
public:
    Status Init(Scene& scene, PhysicsWorld* physicsWorld, const GameplaySettings& settings = {});
    void Shutdown();

    void ResetMatch(Scene& scene);
    void PushInput(const GameplayInputFrame& input);
    void FixedUpdate(Scene& scene, float fixedDeltaSeconds, uint64_t tick);

    std::vector<GameplayEvent> DrainEvents();

    const GameplaySettings& GetSettings() const;
    const GameplayTuning& GetTuning() const;
    bool IsInitialized() const;

private:
    GameplaySettings m_Settings;
    GameplayTuning m_Tuning;
    GameplayInputBuffer m_InputBuffer;
    GameplayEventQueue m_Events;
    PhysicsWorld* m_PhysicsWorld = nullptr;
    bool m_Initialized = false;
};

}
