#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"

#include "Hockey/Gameplay/Match/FaceoffSystem.hpp"
#include "Hockey/Gameplay/Match/MatchClock.hpp"
#include "Hockey/Gameplay/Match/MatchSystem.hpp"
#include "Hockey/Gameplay/Match/ResetSystem.hpp"
#include "Hockey/Gameplay/Player/PlayerMovement.hpp"
#include "Hockey/Gameplay/Puck/PuckPossession.hpp"

namespace Hockey {

Status GameplayWorld::Init(Scene& scene, PhysicsWorld* physicsWorld, const GameplaySettings& settings) {
    m_Settings = settings;
    m_PhysicsWorld = physicsWorld;
    m_InputBuffer.Reset();
    m_Events.Clear();

    const Status status = MatchSystem::InitializeMatch(scene, settings);
    if (!status) {
        m_Initialized = false;
        return status;
    }

    m_Initialized = true;
    m_Events.Push({GameplayEventType::MatchInitialized});
    return Status::Ok();
}

void GameplayWorld::Shutdown() {
    m_InputBuffer.Reset();
    m_Events.Clear();
    m_PhysicsWorld = nullptr;
    m_Initialized = false;
}

void GameplayWorld::ResetMatch(Scene& scene) {
    if (!m_Initialized) {
        return;
    }

    ResetSystem::BeginReset(scene, m_Events);
    ResetSystem::CompleteReset(scene, m_Events);
}

void GameplayWorld::PushInput(const GameplayInputFrame& input) {
    m_InputBuffer.PushInput(input);
}

void GameplayWorld::FixedUpdate(Scene& scene, float fixedDeltaSeconds, uint64_t tick) {
    if (!m_Initialized || !m_Settings.enabled) {
        return;
    }

    FaceoffSystem::FixedUpdate(scene, fixedDeltaSeconds, m_Events);
    PlayerMovement::FixedUpdate(scene, m_PhysicsWorld, m_InputBuffer, m_Tuning, fixedDeltaSeconds);
    PuckPossession::FixedUpdate(scene, m_Events);
    MatchClock::FixedUpdate(scene, fixedDeltaSeconds, m_Events);
    ResetSystem::FixedUpdate(scene, fixedDeltaSeconds, m_Settings, m_Events);
    m_InputBuffer.ClearForTick(tick);
}

std::vector<GameplayEvent> GameplayWorld::DrainEvents() {
    return m_Events.Drain();
}

const GameplaySettings& GameplayWorld::GetSettings() const {
    return m_Settings;
}

const GameplayTuning& GameplayWorld::GetTuning() const {
    return m_Tuning;
}

bool GameplayWorld::IsInitialized() const {
    return m_Initialized;
}

}
