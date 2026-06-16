#include "Test.hpp"

#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/Gameplay/GameplayComponents.hpp"

using namespace Hockey;

void RunGameplayComponentTests() {
    HockeyTest::BeginSuite("GameplayComponentTests");

    ComponentRegistry::Get().RegisterPhase2Components();
    RegisterGameplayComponents();

    HK_CHECK(ComponentRegistry::Get().FindByName("PlayerComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("SkaterComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("GoalieComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("PuckGameplayComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("StickComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("MatchStateComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("TeamStateComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("ScoreComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("PossessionComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("ShotComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("PassComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("CheckComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("FaceoffComponent") != nullptr);
    HK_CHECK(ComponentRegistry::Get().FindByName("RespawnComponent") != nullptr);

    Scene scene("GameplayRoundTrip");
    Entity match = scene.CreateEntity("Match");
    MatchStateComponent matchState;
    matchState.phase = MatchPhase::Playing;
    matchState.period = 2;
    matchState.homeScore = 3;
    matchState.awayScore = 1;
    matchState.clockRunning = true;
    match.AddComponent<MatchStateComponent>(matchState);

    Entity player = scene.CreateEntity("Home Skater");
    PlayerComponent playerComponent;
    playerComponent.playerIndex = 1;
    playerComponent.slot = PlayerSlot::HomeSkater1;
    playerComponent.team = GameplayTeam::Home;
    playerComponent.role = GameplayRole::Skater;
    playerComponent.controlledByLocalInput = true;
    player.AddComponent<PlayerComponent>(playerComponent);
    player.AddComponent<SkaterComponent>().hasPuck = true;
    player.AddComponent<PlayerRuntimeComponent>().velocity = {1.0f, 0.0f, 2.0f};
    player.AddComponent<StickComponent>().ownerPlayer = player.GetUUID();
    player.AddComponent<ShotComponent>().charge = 0.5f;
    player.AddComponent<PassComponent>().targetPlayer = UUID(1234);
    player.AddComponent<CheckComponent>().cooldown = 0.25f;
    player.AddComponent<RespawnComponent>().targetPosition = {3.0f, 0.0f, 4.0f};

    Entity puck = scene.CreateEntity("Puck");
    PuckGameplayComponent puckGameplay;
    puckGameplay.state = PuckState::Possessed;
    puckGameplay.possessingPlayer = player.GetUUID();
    puckGameplay.lastTouchedTeam = GameplayTeam::Home;
    puck.AddComponent<PuckGameplayComponent>(puckGameplay);
    puck.AddComponent<PuckRuntimeComponent>().velocity = {0.0f, 0.0f, 5.0f};
    puck.AddComponent<PossessionComponent>().possessingPlayer = player.GetUUID();

    Entity goal = scene.CreateEntity("Away Goal Gameplay");
    GoalGameplayComponent goalGameplay;
    goalGameplay.scoringTeam = GameplayTeam::Home;
    goalGameplay.defendingTeam = GameplayTeam::Away;
    goal.AddComponent<GoalGameplayComponent>(goalGameplay);
    goal.AddComponent<OutOfPlayComponent>().minY = -6.0f;
    goal.AddComponent<FaceoffGameplayComponent>().centerIce = true;
    goal.AddComponent<FaceoffComponent>().locked = true;

    Entity team = scene.CreateEntity("Home Team State");
    TeamStateComponent teamState;
    teamState.team = GameplayTeam::Home;
    teamState.score = 3;
    teamState.players.push_back(player.GetUUID());
    teamState.goalie = UUID(5678);
    team.AddComponent<TeamStateComponent>(teamState);
    team.AddComponent<ScoreComponent>().homeScore = 3;

    const auto path = Paths::TempFile("gameplay_components_roundtrip.scene.yaml");
    HK_CHECK_MSG(static_cast<bool>(SceneSerializer(scene).Serialize(path)), "gameplay scene saves");

    Scene loaded("Loaded");
    HK_CHECK_MSG(static_cast<bool>(SceneSerializer(loaded).Deserialize(path)), "gameplay scene loads");

    Entity loadedMatch = loaded.FindEntityByName("Match");
    HK_CHECK(loadedMatch.HasComponent<MatchStateComponent>());
    HK_CHECK_EQ(loadedMatch.GetComponent<MatchStateComponent>().phase, MatchPhase::Playing);
    HK_CHECK_EQ(loadedMatch.GetComponent<MatchStateComponent>().homeScore, 3u);

    Entity loadedPlayer = loaded.FindEntityByName("Home Skater");
    HK_CHECK(loadedPlayer.HasComponent<PlayerComponent>());
    HK_CHECK_EQ(loadedPlayer.GetComponent<PlayerComponent>().slot, PlayerSlot::HomeSkater1);
    HK_CHECK_EQ(loadedPlayer.GetComponent<PlayerComponent>().team, GameplayTeam::Home);
    HK_CHECK(loadedPlayer.GetComponent<PlayerComponent>().controlledByLocalInput);
    HK_CHECK(loadedPlayer.GetComponent<SkaterComponent>().hasPuck);
    HK_CHECK_NEAR(loadedPlayer.GetComponent<PlayerRuntimeComponent>().velocity.z, 2.0f, 0.0001f);
    HK_CHECK_EQ(loadedPlayer.GetComponent<StickComponent>().ownerPlayer, loadedPlayer.GetUUID());
    HK_CHECK_NEAR(loadedPlayer.GetComponent<ShotComponent>().charge, 0.5f, 0.0001f);

    Entity loadedPuck = loaded.FindEntityByName("Puck");
    HK_CHECK_EQ(loadedPuck.GetComponent<PuckGameplayComponent>().state, PuckState::Possessed);
    HK_CHECK_EQ(loadedPuck.GetComponent<PuckGameplayComponent>().lastTouchedTeam, GameplayTeam::Home);
    HK_CHECK_NEAR(loadedPuck.GetComponent<PuckRuntimeComponent>().velocity.z, 5.0f, 0.0001f);
    HK_CHECK_EQ(loadedPuck.GetComponent<PossessionComponent>().possessingPlayer, loadedPlayer.GetUUID());

    Entity loadedGoal = loaded.FindEntityByName("Away Goal Gameplay");
    HK_CHECK_EQ(loadedGoal.GetComponent<GoalGameplayComponent>().scoringTeam, GameplayTeam::Home);
    HK_CHECK_EQ(loadedGoal.GetComponent<GoalGameplayComponent>().defendingTeam, GameplayTeam::Away);
    HK_CHECK_NEAR(loadedGoal.GetComponent<OutOfPlayComponent>().minY, -6.0f, 0.0001f);
    HK_CHECK(loadedGoal.GetComponent<FaceoffGameplayComponent>().centerIce);
    HK_CHECK(loadedGoal.GetComponent<FaceoffComponent>().locked);

    Entity loadedTeam = loaded.FindEntityByName("Home Team State");
    HK_CHECK_EQ(loadedTeam.GetComponent<TeamStateComponent>().team, GameplayTeam::Home);
    HK_CHECK_EQ(loadedTeam.GetComponent<TeamStateComponent>().score, 3u);
    HK_CHECK_EQ(loadedTeam.GetComponent<TeamStateComponent>().players.size(), static_cast<std::size_t>(1));
    HK_CHECK_EQ(loadedTeam.GetComponent<ScoreComponent>().homeScore, 3u);
}
