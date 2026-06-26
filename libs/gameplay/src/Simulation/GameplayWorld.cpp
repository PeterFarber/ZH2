#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"

#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/FaceoffSystem.hpp"
#include "Hockey/Gameplay/Match/MatchClock.hpp"
#include "Hockey/Gameplay/Match/MatchSystem.hpp"
#include "Hockey/Gameplay/Match/ResetSystem.hpp"
#include "Hockey/Gameplay/Player/PlayerMovement.hpp"
#include "Hockey/Gameplay/Puck/PuckController.hpp"
#include "Hockey/Gameplay/Puck/PuckPossession.hpp"
#include "Hockey/Gameplay/Rink/GoalDetection.hpp"
#include "Hockey/Gameplay/Rink/OutOfPlaySystem.hpp"
#include "Hockey/Gameplay/Stick/ShootingSystem.hpp"
#include "Hockey/Gameplay/Stick/StealSystem.hpp"

namespace Hockey {
namespace {

bool IsActiveGameplayPhase(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    if (it == view.end()) {
        return true;
    }
    return view.get<MatchStateComponent>(*it).phase == MatchPhase::Playing;
}

} // namespace

Status GameplayWorld::Init(Scene& scene,
                           PhysicsWorld* physicsWorld,
                           const GameplaySettings& settings,
                           const GameplayTuning& tuning) {
    m_Settings = settings;
    m_Tuning = tuning;
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
    ResetMatchForFaceoff(scene, GameplayTeam::None);
}

void GameplayWorld::ResetMatchForFaceoff(Scene& scene, GameplayTeam causeTeam) {
    if (!m_Initialized) {
        return;
    }

    ResetSystem::BeginReset(scene, m_Events, causeTeam);
    ResetSystem::CompleteReset(scene, m_Events, causeTeam, m_Settings, m_PhysicsWorld);
}

void GameplayWorld::PushInput(const GameplayInputFrame& input) {
    m_InputBuffer.PushInput(input);
}

void GameplayWorld::FixedUpdate(Scene& scene, float fixedDeltaSeconds, uint64_t tick) {
    if (!m_Initialized || !m_Settings.enabled) {
        return;
    }

    FaceoffSystem::FixedUpdate(scene, fixedDeltaSeconds, m_Settings, m_Events);
    PlayerMovement::FixedUpdate(scene, m_PhysicsWorld, m_InputBuffer, m_Tuning, fixedDeltaSeconds, m_Events);
    if (IsActiveGameplayPhase(scene)) {
        PuckPossession::FixedUpdate(scene, m_Events, m_PhysicsWorld, m_Tuning.puck.floorY);
        ShootingSystem::FixedUpdate(scene, m_InputBuffer, m_Tuning, fixedDeltaSeconds, m_Events, m_PhysicsWorld);
        StealSystem::FixedUpdate(scene, m_InputBuffer, m_Tuning, fixedDeltaSeconds, m_Events, m_PhysicsWorld);
        PuckController::FixedUpdate(scene, m_Tuning, fixedDeltaSeconds);
        GoalDetection::FixedUpdate(scene, m_Settings, m_Events);
        OutOfPlaySystem::HandleOutOfPlay(scene, m_Settings, m_Events);
    }
    MatchClock::FixedUpdate(scene, fixedDeltaSeconds, m_Settings, m_Events);
    ResetSystem::FixedUpdate(scene, fixedDeltaSeconds, m_Settings, m_Events, m_PhysicsWorld);
    m_InputBuffer.ClearForTick(tick);
}

void GameplayWorld::SyncPhysicsState(Scene& scene) {
    if (!m_Initialized || m_PhysicsWorld == nullptr) {
        return;
    }
    PlayerMovement::SyncFromPhysics(scene, m_PhysicsWorld);
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
