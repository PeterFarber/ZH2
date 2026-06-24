#include "Test.hpp"

#include <entt/entt.hpp>
#include <string>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Puck/PuckPossession.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"
#include "Hockey/Gameplay/Stick/StealSystem.hpp"
#include "Hockey/Gameplay/Stick/StickHandling.hpp"

using namespace Hockey;

namespace {

Entity AddMarker(Scene& scene, const std::string& name, const glm::vec3& position) {
    Entity entity = scene.CreateEntity(name);
    entity.GetComponent<TransformComponent>().localPosition = position;
    return entity;
}

void AddSpawn(Scene& scene, Team team, const glm::vec3& position, bool faceoffSpawn = false) {
    Entity spawn = AddMarker(scene, "Spawn", position);
    SpawnPointComponent component;
    component.team = team;
    component.faceoffSpawn = faceoffSpawn;
    spawn.AddComponent<SpawnPointComponent>(component);
}

void BuildValidGameplayScene(Scene& scene) {
    Entity rink = AddMarker(scene, "Rink", {0.0f, 0.0f, 0.0f});
    rink.AddComponent<RinkComponent>();
    rink.AddComponent<PlayAreaComponent>();

    AddMarker(scene, "Puck", {0.0f, 0.05f, 0.0f}).AddComponent<PuckComponent>();
    AddMarker(scene, "Home Goal", {0.0f, 0.0f, -28.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Home;
    AddMarker(scene, "Away Goal", {0.0f, 0.0f, 28.0f}).AddComponent<GoalComponent>().defendingTeam = Team::Away;
    for (int i = 0; i < 4; ++i) {
        AddSpawn(scene, Team::Home, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, -5.0f});
        AddSpawn(scene, Team::Away, {-6.0f + static_cast<float>(i) * 4.0f, 0.0f, 5.0f});
    }
    for (int i = 0; i < 8; ++i) {
        AddSpawn(scene, Team::None, {-14.0f + static_cast<float>(i) * 4.0f, 0.0f, 0.0f}, true);
    }
}

Entity FindPlayer(Scene& scene, PlayerSlot slot) {
    auto view = scene.Registry().view<PlayerComponent>();
    for (const entt::entity handle : view) {
        Entity player(handle, &scene);
        if (player.GetComponent<PlayerComponent>().slot == slot) {
            return player;
        }
    }
    return {};
}

Entity FindPuck(Scene& scene) {
    auto view = scene.Registry().view<PuckComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

Entity FindMatch(Scene& scene) {
    auto view = scene.Registry().view<MatchStateComponent>();
    const auto it = view.begin();
    return it == view.end() ? Entity{} : Entity(*it, &scene);
}

bool SawEvent(const std::vector<GameplayEvent>& events, GameplayEventType type) {
    for (const GameplayEvent& event : events) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
}

void PushSteal(GameplayWorld& world, uint32_t playerIndex, uint64_t tick) {
    GameplayInputFrame input;
    input.playerIndex = playerIndex;
    input.inputSequence = tick;
    input.simulationTick = tick;
    input.stealPressed = true;
    world.PushInput(input);
}

} // namespace

void RunStealTests() {
    HockeyTest::BeginSuite("StealTests");

    {
        Scene scene("StealTransferScene");
        BuildValidGameplayScene(scene);

        GameplaySettings settings;
        GameplayWorld world;
        HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
        world.DrainEvents();
        FindMatch(scene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;

        Entity stealer = FindPlayer(scene, PlayerSlot::HomeSkater0);
        Entity carrier = FindPlayer(scene, PlayerSlot::AwaySkater0);
        Entity puck = FindPuck(scene);
        stealer.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};
        stealer.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
        carrier.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 1.0f};
        carrier.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, -1.0f};
        puck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(carrier);

        GameplayEventQueue events;
        HK_CHECK(PuckPossession::TryAcquire(scene, carrier, puck, events));
        puck.GetComponent<TransformComponent>().localPosition = StickHandling::GetStickWorldPosition(stealer);

        PushSteal(world, stealer.GetComponent<PlayerComponent>().playerIndex, 1);
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
        const std::vector<GameplayEvent> stealEvents = world.DrainEvents();

        HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Possessed);
        HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().possessingPlayer, stealer.GetUUID());
        HK_CHECK(!carrier.GetComponent<SkaterComponent>().hasPuck);
        HK_CHECK(stealer.GetComponent<SkaterComponent>().hasPuck);
        HK_CHECK(stealer.GetComponent<PlayerRuntimeComponent>().stealCooldown > 0.0f);
        HK_CHECK(SawEvent(stealEvents, GameplayEventType::StealAttempted));
        HK_CHECK(SawEvent(stealEvents, GameplayEventType::PuckPossessionChanged));

        PushSteal(world, stealer.GetComponent<PlayerComponent>().playerIndex, 2);
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, 2);
        HK_CHECK(!SawEvent(world.DrainEvents(), GameplayEventType::StealAttempted));
    }

    {
        Scene scene("FailedStealCooldownScene");
        BuildValidGameplayScene(scene);

        GameplaySettings settings;
        GameplayWorld world;
        HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
        world.DrainEvents();
        FindMatch(scene).GetComponent<MatchStateComponent>().phase = MatchPhase::Playing;

        Entity stealer = FindPlayer(scene, PlayerSlot::HomeSkater0);
        Entity target = FindPlayer(scene, PlayerSlot::AwaySkater0);
        Entity puck = FindPuck(scene);
        stealer.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 0.0f};
        stealer.GetComponent<PlayerRuntimeComponent>().facingDirection = {0.0f, 0.0f, 1.0f};
        target.GetComponent<TransformComponent>().localPosition = {0.0f, 0.0f, 1.0f};
        puck.GetComponent<TransformComponent>().localPosition = {10.0f, 0.0f, 10.0f};

        PushSteal(world, stealer.GetComponent<PlayerComponent>().playerIndex, 1);
        world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);

        HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Loose);
        HK_CHECK_EQ(target.GetComponent<PlayerRuntimeComponent>().velocity, glm::vec3{0.0f});
        HK_CHECK(stealer.GetComponent<PlayerRuntimeComponent>().stealCooldown > 0.0f);
        HK_CHECK(SawEvent(world.DrainEvents(), GameplayEventType::StealAttempted));
    }

    {
        Scene scene("LoosePuckStealDoesNotPokeScene");
        Entity stealer = AddMarker(scene, "Stealer", {0.0f, 0.0f, 0.0f});
        PlayerComponent player;
        player.playerIndex = 0;
        player.team = GameplayTeam::Home;
        stealer.AddComponent<PlayerComponent>(player);
        stealer.AddComponent<SkaterComponent>();
        stealer.AddComponent<PlayerRuntimeComponent>();
        stealer.AddComponent<StickComponent>().ownerPlayer = stealer.GetUUID();

        Entity puck = AddMarker(scene, "Puck", {0.0f, 0.0f, 1.0f});
        puck.AddComponent<PuckComponent>();
        puck.AddComponent<PuckGameplayComponent>();
        puck.AddComponent<PuckRuntimeComponent>();

        GameplayInputBuffer inputs;
        GameplayInputFrame input;
        input.playerIndex = 0;
        input.stealPressed = true;
        inputs.PushInput(input);

        GameplayTuning tuning;
        GameplayEventQueue events;
        StealSystem::FixedUpdate(scene, inputs, tuning, 1.0f / 60.0f, events);

        HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Loose);
        HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().possessingPlayer, UUID(0));
        HK_CHECK_EQ(puck.GetComponent<PuckRuntimeComponent>().velocity, glm::vec3{0.0f});
        HK_CHECK(SawEvent(events.Drain(), GameplayEventType::StealAttempted));
    }
}
