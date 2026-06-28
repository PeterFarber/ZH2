#include "Test.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include <entt/entt.hpp>
#include <glm/geometric.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"
#include "Hockey/Gameplay/Match/ResetSystem.hpp"
#include "Hockey/Gameplay/Match/ScoreSystem.hpp"
#include "Hockey/Gameplay/Simulation/GameplayWorld.hpp"

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
        AddSpawn(scene, Team::None, {-14.0f + static_cast<float>(i) * 4.0f, 0.0f, 2.0f}, true);
        AddSpawn(scene, Team::Home, {-14.0f + static_cast<float>(i) * 4.0f, 0.0f, -30.0f}, true);
        AddSpawn(scene, Team::Away, {-14.0f + static_cast<float>(i) * 4.0f, 0.0f, 30.0f}, true);
    }
}

bool SawEvent(const std::vector<GameplayEvent>& events, GameplayEventType type) {
    for (const GameplayEvent& event : events) {
        if (event.type == type) {
            return true;
        }
    }
    return false;
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

glm::vec3 ExpectedFacingToward(const glm::vec3& from, const glm::vec3& target) {
    glm::vec3 direction = target - from;
    direction.y = 0.0f;
    return glm::length(direction) > 0.0001f ? glm::normalize(direction) : glm::vec3{0.0f, 0.0f, 1.0f};
}

float ExpectedYawDegrees(const glm::vec3& facing) {
    constexpr float kRadiansToDegrees = 57.29577951308232f;
    return std::atan2(facing.x, facing.z) * kRadiansToDegrees;
}

void CheckPlayerFacesPuck(Entity player, Entity puck) {
    const TransformComponent& playerTransform = player.GetComponent<TransformComponent>();
    const glm::vec3 expectedFacing =
        ExpectedFacingToward(playerTransform.localPosition, puck.GetComponent<TransformComponent>().localPosition);
    const PlayerRuntimeComponent& runtime = player.GetComponent<PlayerRuntimeComponent>();

    HK_CHECK_NEAR(runtime.facingDirection.x, expectedFacing.x, 0.001f);
    HK_CHECK_NEAR(runtime.facingDirection.y, 0.0f, 0.001f);
    HK_CHECK_NEAR(runtime.facingDirection.z, expectedFacing.z, 0.001f);
    HK_CHECK_NEAR(playerTransform.localRotation.x, 0.0f, 0.001f);
    HK_CHECK_NEAR(playerTransform.localRotation.y, ExpectedYawDegrees(expectedFacing), 0.001f);
    HK_CHECK_NEAR(playerTransform.localRotation.z, 0.0f, 0.001f);
}

void CheckAllPlayersFacePuck(Scene& scene) {
    Entity puck = FindPuck(scene);
    HK_CHECK(puck.IsValid());
    auto view = scene.Registry().view<PlayerComponent, PlayerRuntimeComponent, TransformComponent>();
    int checked = 0;
    for (const entt::entity handle : view) {
        CheckPlayerFacesPuck(Entity(handle, &scene), puck);
        ++checked;
    }
    HK_CHECK_EQ(checked, 8);
}

bool AllPlayersInZBand(Scene& scene, float expectedZ) {
    auto view = scene.Registry().view<PlayerComponent, TransformComponent>();
    int count = 0;
    for (const entt::entity handle : view) {
        const glm::vec3 position = view.get<TransformComponent>(handle).localPosition;
        if (std::abs(position.z - expectedZ) > 0.001f) {
            return false;
        }
        ++count;
    }
    return count == 8;
}

bool AllPlayersForTeamInZBand(Scene& scene, GameplayTeam team, float expectedZ) {
    auto view = scene.Registry().view<PlayerComponent, TransformComponent>();
    int count = 0;
    for (const entt::entity handle : view) {
        const PlayerComponent& player = view.get<PlayerComponent>(handle);
        if (player.team != team) {
            continue;
        }
        const glm::vec3 position = view.get<TransformComponent>(handle).localPosition;
        if (std::abs(position.z - expectedZ) > 0.001f) {
            return false;
        }
        ++count;
    }
    return count == 4;
}

void ConfigureNeutralFaceoffSlots(Scene& scene) {
    std::vector<Entity> spawns;
    auto view = scene.Registry().view<SpawnPointComponent, TransformComponent>();
    for (const entt::entity handle : view) {
        const SpawnPointComponent& spawn = view.get<SpawnPointComponent>(handle);
        if (spawn.faceoffSpawn && spawn.team == Team::None) {
            spawns.emplace_back(handle, &scene);
        }
    }
    std::sort(spawns.begin(), spawns.end(), [](Entity lhs, Entity rhs) {
        return lhs.GetComponent<TransformComponent>().localPosition.x <
               rhs.GetComponent<TransformComponent>().localPosition.x;
    });

    for (std::size_t i = 0; i < spawns.size() && i < 8; ++i) {
        spawns[i].AddOrReplaceComponent<TeamComponent>(
            TeamComponent{i < 4 ? Team::Home : Team::Away});
        spawns[i].AddOrReplaceComponent<PlayerRoleComponent>(
            PlayerRoleComponent{(i == 3 || i == 7) ? PlayerRole::Goalie : PlayerRole::Skater});
    }
}

bool PlayerNearX(Scene& scene, PlayerSlot slot, float expectedX) {
    Entity player = FindPlayer(scene, slot);
    return player.IsValid() &&
           std::abs(player.GetComponent<TransformComponent>().localPosition.x - expectedX) <= 0.001f;
}

} // namespace

void RunResetSystemTests() {
    HockeyTest::BeginSuite("ResetSystemTests");

    {
        Scene scene("ImmediateResetScene");
        BuildValidGameplayScene(scene);

        GameplayWorld world;
        HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, {})), "gameplay world initializes");
        world.DrainEvents();

        Entity player = FindPlayer(scene, PlayerSlot::HomeSkater1);
        Entity puck = FindPuck(scene);
        HK_CHECK(player.IsValid());
        HK_CHECK(puck.IsValid());

        player.GetComponent<TransformComponent>().localPosition = {20.0f, 0.0f, 20.0f};
        player.GetComponent<PlayerRuntimeComponent>().velocity = {5.0f, 0.0f, 0.0f};
        player.GetComponent<SkaterComponent>().hasPuck = true;
        puck.GetComponent<TransformComponent>().localPosition = {12.0f, -2.0f, 4.0f};
        puck.GetComponent<PuckGameplayComponent>().state = PuckState::Possessed;
        puck.GetComponent<PuckGameplayComponent>().possessingPlayer = player.GetUUID();
        puck.GetComponent<PuckRuntimeComponent>().velocity = {3.0f, 0.0f, 1.0f};

        world.ResetMatch(scene);
        std::vector<GameplayEvent> events = world.DrainEvents();

        HK_CHECK(SawEvent(events, GameplayEventType::ResetStarted));
        HK_CHECK(SawEvent(events, GameplayEventType::ResetCompleted));
        HK_CHECK_EQ(FindMatch(scene).GetComponent<MatchStateComponent>().phase, MatchPhase::FaceoffSetup);
        const glm::vec3 zero{0.0f};
        HK_CHECK_NEAR(player.GetComponent<TransformComponent>().localPosition.z, 2.0f, 0.001f);
        HK_CHECK_EQ(player.GetComponent<PlayerRuntimeComponent>().velocity, zero);
        HK_CHECK(!player.GetComponent<SkaterComponent>().hasPuck);
        const glm::vec3 expectedPuckPosition{0.0f, 0.0f, 2.0f};
        HK_CHECK_EQ(puck.GetComponent<TransformComponent>().localPosition, expectedPuckPosition);
        CheckAllPlayersFacePuck(scene);
        HK_CHECK_EQ(puck.GetComponent<PuckGameplayComponent>().state, PuckState::Loose);
        HK_CHECK(!puck.GetComponent<PuckGameplayComponent>().possessingPlayer.IsValid());
        HK_CHECK_EQ(puck.GetComponent<PuckRuntimeComponent>().velocity, zero);
    }

    {
        Scene scene("DelayedResetScene");
        BuildValidGameplayScene(scene);

        GameplaySettings settings;
        settings.postGoalDelaySeconds = settings.fixedDeltaSeconds * 2.0f;
        GameplayWorld world;
        HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
        world.DrainEvents();

        GameplayEventQueue events;
        ResetSystem::BeginReset(scene, events);
        HK_CHECK(SawEvent(events.Drain(), GameplayEventType::ResetStarted));
        HK_CHECK_EQ(FindMatch(scene).GetComponent<MatchStateComponent>().phase, MatchPhase::GoalScored);

        world.FixedUpdate(scene, settings.fixedDeltaSeconds, 1);
        HK_CHECK_EQ(FindMatch(scene).GetComponent<MatchStateComponent>().phase, MatchPhase::GoalScored);

        world.FixedUpdate(scene, settings.fixedDeltaSeconds, 2);
        HK_CHECK_EQ(FindMatch(scene).GetComponent<MatchStateComponent>().phase, MatchPhase::FaceoffSetup);
        HK_CHECK(SawEvent(world.DrainEvents(), GameplayEventType::ResetCompleted));
    }

    {
        Scene scene("PenaltyFaceoffResetScene");
        BuildValidGameplayScene(scene);

        GameplayWorld world;
        GameplaySettings settings;
        settings.spawnRandomSeed = 1234u;
        HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
        world.DrainEvents();

        world.ResetMatchForFaceoff(scene, GameplayTeam::Home);
        HK_CHECK(AllPlayersInZBand(scene, -30.0f));

        world.ResetMatchForFaceoff(scene, GameplayTeam::Away);
        HK_CHECK(AllPlayersInZBand(scene, 30.0f));
    }

    {
        Scene scene("PostGoalNormalSpawnResetScene");
        BuildValidGameplayScene(scene);

        GameplayWorld world;
        GameplaySettings settings;
        settings.postGoalDelaySeconds = settings.fixedDeltaSeconds;
        HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
        world.DrainEvents();

        auto players = scene.Registry().view<PlayerComponent, TransformComponent>();
        for (const entt::entity handle : players) {
            players.get<TransformComponent>(handle).localPosition = {20.0f, 0.0f, 20.0f};
        }

        GameplayEventQueue events;
        ScoreSystem::AddGoal(scene, GameplayTeam::Home, settings, events);
        ResetSystem::FixedUpdate(scene, settings.fixedDeltaSeconds, settings, events);

        HK_CHECK(AllPlayersForTeamInZBand(scene, GameplayTeam::Home, -5.0f));
        HK_CHECK(AllPlayersForTeamInZBand(scene, GameplayTeam::Away, 5.0f));
    }

    {
        Scene scene("NeutralFaceoffSlotMetadataResetScene");
        BuildValidGameplayScene(scene);
        ConfigureNeutralFaceoffSlots(scene);

        GameplayWorld world;
        GameplaySettings settings;
        settings.spawnRandomSeed = 0u;
        HK_CHECK_MSG(static_cast<bool>(world.Init(scene, nullptr, settings)), "gameplay world initializes");
        world.DrainEvents();

        world.ResetMatchForFaceoff(scene, GameplayTeam::None);

        HK_CHECK(PlayerNearX(scene, PlayerSlot::HomeGoalie, -2.0f));
        HK_CHECK(PlayerNearX(scene, PlayerSlot::AwayGoalie, 14.0f));
    }
}
