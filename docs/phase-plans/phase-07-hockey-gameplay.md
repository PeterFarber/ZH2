# Phase 7 Plan — Complete Hockey Gameplay Simulation

File path recommendation:

```text
docs/phase-plans/phase-07-hockey-gameplay.md
```

AI instruction:

```text
Read `GAMEPLAY.md` and `docs/phase-plans/phase-07-hockey-gameplay.md`, then implement Phase 7 exactly. Build the complete local/headless hockey gameplay simulation on top of the existing core, ECS, asset, renderer, editor, and physics systems. Do not implement GameNetworkingSockets transport, online lobby networking, replication protocol, final animation runtime, final audio runtime, or final game UI during this phase. Gameplay must be server-authoritative-ready, fixed-tick, deterministic-minded, and independent from SDL, Vulkan, ImGui, and networking transport. Keep the dedicated server headless and keep Windows/Linux compatibility.
```

---

# 0. Phase Summary

Phase 7 builds the complete hockey gameplay simulation.

This phase turns the project from a scene/render/physics/editor framework into a playable local and headless hockey simulation with:

```text
4v4 match setup
2 teams
3 skaters + 1 goalie per team
8 players total
1 puck
2 goals
team assignment
role assignment
spawn assignment
faceoffs
pregame countdown
period clock with 3 periods at 180 seconds each
score
goal detection
reset flow
player movement model
goalie movement model
waypoint movement and clearing
skater boost impulses
goalie boost charges
goalie shield
stick handling
puck possession/contact model
contextual steal/shoot actions
shooting
passing
checking/body-contact hooks
role-specific collider/trigger authoring
out-of-play handling
semantic gameplay input model independent from SDL
server-authoritative-ready fixed tick simulation
editor validation and preview hooks
client local play path
server headless simulation path
gameplay tests
```

Phase 7 must produce:

```text
hockey_gameplay
HockeyGameplayTests
```

Phase 7 updates:

```text
HockeyGameClient
HockeyMapEditor
HockeyDedicatedServer
hockey_ecs component serialization/metadata
hockey_physics usage/integration
hockey_editor validation/tools where needed
data/config
data/gameplay
data/raw/scenes
data/raw/prefabs
```

Phase 7 must not implement:

```text
GameNetworkingSockets transport
real online lobbies
replication protocol
client prediction/reconciliation netcode
final animation graph/runtime
final audio system
final polished game UI/HUD
```

This phase must make the gameplay simulation complete enough that Phase 8 can wrap it in networking rather than rewriting it.

`GAMEPLAY.md` is the player-facing gameplay design source for action behavior. This plan owns implementation shape, dependency boundaries, tests, and phase verification. When these files differ, update the plan/rules/status in the same change as the gameplay design update.

---

# 1. Architecture Rules

## 1.1 Dependency Direction

Add:

```text
hockey_gameplay -> hockey_core, hockey_ecs, hockey_physics
```

Optional:

```text
hockey_gameplay -> hockey_assets
```

Only use `hockey_assets` if gameplay tuning/assets are loaded through the asset system.

Allowed app links:

```text
HockeyGameClient       -> hockey_gameplay
HockeyMapEditor        -> hockey_gameplay
HockeyDedicatedServer  -> hockey_gameplay
HockeyGameplayTests    -> hockey_gameplay
```

Allowed editor link:

```text
hockey_editor -> hockey_gameplay
```

Only for:

```text
hockey scene validation
gameplay marker editing helpers
local gameplay preview controls
```

Forbidden dependencies:

```text
hockey_gameplay -> hockey_renderer
hockey_gameplay -> hockey_editor
hockey_gameplay -> hockey_networking
hockey_gameplay -> SDL3
hockey_gameplay -> Vulkan
hockey_gameplay -> ImGui
hockey_gameplay -> GameNetworkingSockets
```

## 1.2 Gameplay Owns

`hockey_gameplay` owns:

```text
match state
team state
player role state
hockey input data model
spawn assignment
faceoff logic
score logic
period clock
goal detection interpretation
puck state
player/skater/goalie simulation
stick interaction model
shot/pass/check gameplay logic
out-of-play handling
gameplay event queue
gameplay scene validation
gameplay tuning data
server-authoritative simulation rules
```

## 1.3 Gameplay Must Not Own

`hockey_gameplay` must not own:

```text
SDL input polling
Vulkan rendering
ImGui UI
network sockets
packet serialization
lobby transport
final animation playback
audio playback
asset importing/cooking
```

## 1.4 Server Rule

Dedicated server must:

```text
link gameplay and physics
run gameplay headlessly
own authoritative match simulation
not require renderer
not require editor
not require UI
not require GPU/display
```

## 1.5 Input Boundary

Gameplay receives input as data:

```text
GameplayInputFrame
```

The game client translates keyboard/mouse/gamepad input into gameplay input data.

Gameplay must not directly read:

```text
SDL events
keyboard state
mouse state
gamepad state
```

---

# 2. Target Project Structure

Add:

```text
libs/
  gameplay/
    CMakeLists.txt
    include/Hockey/Gameplay/
      Gameplay.hpp
      GameplaySettings.hpp
      GameplayTypes.hpp
      GameplayInput.hpp
      GameplayEvents.hpp

      Match/
        MatchState.hpp
        MatchClock.hpp
        MatchRules.hpp
        MatchSystem.hpp
        ScoreSystem.hpp
        PeriodSystem.hpp
        FaceoffSystem.hpp
        ResetSystem.hpp

      Teams/
        TeamTypes.hpp
        TeamState.hpp
        TeamAssignment.hpp
        RoleAssignment.hpp

      Player/
        PlayerComponents.hpp
        PlayerInput.hpp
        PlayerController.hpp
        SkaterController.hpp
        GoalieController.hpp
        PlayerMovement.hpp
        PlayerSpawnSystem.hpp

      Puck/
        PuckComponents.hpp
        PuckController.hpp
        PuckPossession.hpp
        PuckInteraction.hpp
        PuckResetSystem.hpp

      Stick/
        StickComponents.hpp
        StickHandling.hpp
        ShootingSystem.hpp
        PassingSystem.hpp
        CheckingSystem.hpp

      Rink/
        RinkGameplayComponents.hpp
        GoalDetection.hpp
        OutOfPlaySystem.hpp
        RinkValidation.hpp

      Simulation/
        GameplayWorld.hpp
        GameplaySystem.hpp
        GameplayScene.hpp
        GameplaySnapshot.hpp

      Tuning/
        GameplayTuning.hpp
        TuningSerializer.hpp

      Validation/
        GameplayValidation.hpp

    src/
      Gameplay.cpp
      GameplaySettings.cpp
      GameplayTypes.cpp
      GameplayInput.cpp
      GameplayEvents.cpp

      Match/
        MatchState.cpp
        MatchClock.cpp
        MatchRules.cpp
        MatchSystem.cpp
        ScoreSystem.cpp
        PeriodSystem.cpp
        FaceoffSystem.cpp
        ResetSystem.cpp

      Teams/
        TeamTypes.cpp
        TeamState.cpp
        TeamAssignment.cpp
        RoleAssignment.cpp

      Player/
        PlayerComponents.cpp
        PlayerInput.cpp
        PlayerController.cpp
        SkaterController.cpp
        GoalieController.cpp
        PlayerMovement.cpp
        PlayerSpawnSystem.cpp

      Puck/
        PuckComponents.cpp
        PuckController.cpp
        PuckPossession.cpp
        PuckInteraction.cpp
        PuckResetSystem.cpp

      Stick/
        StickComponents.cpp
        StickHandling.cpp
        ShootingSystem.cpp
        PassingSystem.cpp
        CheckingSystem.cpp

      Rink/
        RinkGameplayComponents.cpp
        GoalDetection.cpp
        OutOfPlaySystem.cpp
        RinkValidation.cpp

      Simulation/
        GameplayWorld.cpp
        GameplaySystem.cpp
        GameplayScene.cpp
        GameplaySnapshot.cpp

      Tuning/
        GameplayTuning.cpp
        TuningSerializer.cpp

      Validation/
        GameplayValidation.cpp

apps/
  gameplay_tests/
    CMakeLists.txt
    src/
      GameplayTestsMain.cpp
      Test.hpp
      MatchSetupTests.cpp
      TeamAssignmentTests.cpp
      RoleAssignmentTests.cpp
      SpawnSystemTests.cpp
      InputModelTests.cpp
      SkaterMovementTests.cpp
      GoalieMovementTests.cpp
      PuckInteractionTests.cpp
      StickHandlingTests.cpp
      ShootingTests.cpp
      PassingTests.cpp
      CheckingTests.cpp
      GoalDetectionTests.cpp
      ScoreTests.cpp
      FaceoffTests.cpp
      PeriodClockTests.cpp
      ResetSystemTests.cpp
      GameplayValidationTests.cpp
      GameplaySnapshotTests.cpp
      HeadlessServerGameplayTests.cpp

data/
  gameplay/
    tuning.default.yaml
    rules.default.yaml

  raw/
    scenes/
      main_rink.scene.yaml
    prefabs/
      skater.prefab.yaml
      goalie.prefab.yaml
      puck.prefab.yaml
      goal.prefab.yaml
```

---

# 3. CMake Requirements

## 3.1 Root CMake

Add:

```cmake
add_subdirectory(libs/gameplay)
```

When tests are enabled:

```cmake
add_subdirectory(apps/gameplay_tests)
```

Ordering:

```text
core
ecs
assets
renderer
physics
gameplay
editor
apps
```

## 3.2 `libs/gameplay/CMakeLists.txt`

Create target:

```text
hockey_gameplay
```

Required links:

```text
hockey_core
hockey_ecs
hockey_physics
```

Optional:

```text
hockey_assets
```

Do not link:

```text
hockey_renderer
hockey_editor
hockey_networking
SDL3
Vulkan
ImGui
GameNetworkingSockets
```

## 3.3 App Link Updates

Update:

```text
HockeyGameClient links hockey_gameplay
HockeyMapEditor links hockey_gameplay
HockeyDedicatedServer links hockey_gameplay
HockeyGameplayTests links hockey_gameplay
```

## 3.4 Editor Link Update

Update if needed:

```text
hockey_editor links hockey_gameplay
```

Only for validation and preview tooling.

---

# 4. Gameplay Settings and Tuning

## 4.1 `GameplaySettings`

Files:

```text
GameplaySettings.hpp/cpp
```

Implement:

```cpp
struct GameplaySettings {
    bool enabled = true;

    uint32_t targetPlayerCount = 8;
    uint32_t skatersPerTeam = 3;
    uint32_t goaliesPerTeam = 1;

    float fixedDeltaSeconds = 1.0f / 60.0f;

    float periodLengthSeconds = 180.0f;
    uint32_t periodCount = 3;
    float pregameCountdownSeconds = 10.0f;
    float countdownBeepStartSeconds = 4.0f;

    bool stopClockAfterGoal = true;
    bool autoFaceoffAfterGoal = true;
    float postGoalDelaySeconds = 3.0f;

    bool allowBodyChecking = true;
    bool allowManualGoalie = true;
    bool allowOutOfPlay = true;

    bool debugDrawGameplay = false;
    bool logGameplayEvents = false;
};
```

Config additions.

`client.toml`:

```toml
[gameplay]
enabled = true
local_play = true
debug_draw_gameplay = false
log_gameplay_events = false
```

`editor.toml`:

```toml
[gameplay]
preview_enabled = false
validate_on_load = true
debug_draw_gameplay = true
```

`server.toml`:

```toml
[gameplay]
enabled = true
authoritative = true
target_player_count = 8
skaters_per_team = 3
goalies_per_team = 1
period_length_seconds = 180.0
period_count = 3
pregame_countdown_seconds = 10.0
countdown_beep_start_seconds = 4.0
log_gameplay_events = true
```

## 4.2 Gameplay Tuning

Files:

```text
Tuning/GameplayTuning.hpp/cpp
Tuning/TuningSerializer.hpp/cpp
```

Create:

```text
data/gameplay/tuning.default.yaml
data/gameplay/rules.default.yaml
```

Tuning should include:

```yaml
Skater:
  MaxSpeed: 9.0
  Acceleration: 32.0
  Deceleration: 20.0
  TurnSpeedDegrees: 720.0
  BoostImpulse: 7.5
  BoostCooldownSeconds: 1.25
  SlideStopDamping: 0.35
  DoubleStopWindowSeconds: 0.30
  PuckControlRadius: 1.25
  StealRadius: 1.5
  StealCooldownSeconds: 0.35

Goalie:
  MaxSpeed: 6.5
  Acceleration: 28.0
  CreaseMoveMultiplier: 1.0
  SaveReachRadius: 1.8
  BoostCharges: 2
  BoostRechargeSeconds: 4.0
  BoostImpulse: 8.0
  ShieldRadius: 2.0
  ShieldDurationSeconds: 1.0
  ShieldCooldownSeconds: 5.0
  ShieldReflectImpulse: 22.0

Puck:
  MaxSpeed: 45.0
  PossessionOffset: [0.0, 0.0, 1.1]
  LoosePuckDrag: 0.15
  OutOfPlayY: -5.0

Stick:
  Reach: 1.5
  Width: 0.25
  PokeCheckCooldown: 0.35

Shot:
  MinPower: 8.0
  MaxPower: 32.0
  ChargeSeconds: 1.0
  AccuracyDegrees: 4.0

Pass:
  Power: 18.0
  AssistRadius: 2.0
  MaxAssistAngleDegrees: 25.0

Check:
  Cooldown: 0.75
  Impulse: 8.0
  Radius: 1.25
```

Rules should include:

```yaml
Match:
  PeriodCount: 3
  PeriodLengthSeconds: 180.0
  PregameCountdownSeconds: 10.0
  CountdownBeepStartSeconds: 4.0
  SkatersPerTeam: 3
  GoaliesPerTeam: 1

Faceoff:
  DelaySeconds: 1.0
  LockPlayersSeconds: 0.5

Goal:
  PostGoalDelaySeconds: 3.0
  RequirePuckTrigger: true
```

Gameplay values are tunable defaults, not final balancing.

---

# 5. Core Gameplay Types

## 5.1 Team and Role Types

Files:

```text
Teams/TeamTypes.hpp/cpp
```

Use or bridge existing ECS marker enums.

Implement gameplay-facing types:

```cpp
enum class GameplayTeam {
    None,
    Home,
    Away
};

enum class GameplayRole {
    Skater,
    Goalie
};

enum class PlayerSlot {
    HomeSkater0,
    HomeSkater1,
    HomeSkater2,
    HomeGoalie,
    AwaySkater0,
    AwaySkater1,
    AwaySkater2,
    AwayGoalie,
    None
};
```

Helpers:

```cpp
GameplayTeam OppositeTeam(GameplayTeam team);
bool IsHomeSlot(PlayerSlot slot);
bool IsAwaySlot(PlayerSlot slot);
bool IsGoalieSlot(PlayerSlot slot);
bool IsSkaterSlot(PlayerSlot slot);
```

## 5.2 Match State

Files:

```text
Match/MatchState.hpp/cpp
```

Implement:

```cpp
enum class MatchPhase {
    NotStarted,
    Warmup,
    FaceoffSetup,
    Faceoff,
    Playing,
    GoalScored,
    PeriodEnded,
    MatchEnded,
    Paused
};

struct MatchStateComponent {
    MatchPhase phase = MatchPhase::NotStarted;

    uint32_t period = 1;
    uint32_t periodCount = 3;

    float periodTimeRemaining = 180.0f;
    float phaseTimer = 0.0f;

    uint32_t homeScore = 0;
    uint32_t awayScore = 0;

    bool clockRunning = false;
    bool matchInitialized = false;
};
```

`MatchPhase::Warmup` represents the 10-second pregame countdown. It locks
player movement and puck interaction, emits countdown tick/beep events, then
transitions to `FaceoffSetup`.

## 5.3 Team State

Files:

```text
Teams/TeamState.hpp/cpp
```

Implement:

```cpp
struct TeamStateComponent {
    GameplayTeam team = GameplayTeam::None;
    uint32_t score = 0;
    std::vector<UUID> players;
    UUID goalie;
};
```

## 5.4 Gameplay Events

Files:

```text
GameplayEvents.hpp/cpp
```

Implement:

```cpp
enum class GameplayEventType {
    MatchInitialized,
    MatchStarted,
    CountdownTick,
    CountdownBeep,
    PeriodStarted,
    PeriodEnded,
    FaceoffStarted,
    FaceoffEnded,
    GoalScored,
    ScoreChanged,
    PuckPossessionChanged,
    StealAttempted,
    PlayerBoosted,
    GoalieShieldStarted,
    GoalieShieldEnded,
    PuckShot,
    PuckPassed,
    PlayerChecked,
    PuckOutOfPlay,
    ResetStarted,
    ResetCompleted,
    MatchEnded
};

struct GameplayEvent {
    GameplayEventType type;
    UUID primaryEntity;
    UUID secondaryEntity;
    GameplayTeam team = GameplayTeam::None;
    glm::vec3 position{0.0f};
    std::string message;
};
```

`GameplayWorld` should collect/drain gameplay events.

---

# 6. Gameplay Input Model

## 6.1 Files

```text
GameplayInput.hpp/cpp
Player/PlayerInput.hpp/cpp
```

## 6.2 Input Frame

Implement:

```cpp
struct GameplayInputFrame {
    uint32_t playerIndex = 0;
    uint64_t inputSequence = 0;
    uint64_t simulationTick = 0;

    glm::vec2 move{0.0f};
    glm::vec2 aim{0.0f};
    glm::vec3 moveTarget{0.0f};

    bool setMoveTarget = false;
    bool clearMoveTarget = false;

    bool stealPressed = false;
    bool stealHeld = false;
    bool stealReleased = false;

    bool boostPressed = false;
    bool brakePressed = false;
    bool brakeHeld = false;
    bool quickTurnPressed = false;

    bool shootPressed = false;
    bool shootHeld = false;
    bool shootReleased = false;

    bool passPressed = false;
    bool passHeld = false;
    bool passReleased = false;

    bool checkPressed = false;
    bool pokeCheckPressed = false;
    bool switchPlayerPressed = false;

    bool goalieShieldPressed = false;
    bool pausePressed = false;
};
```

## 6.3 Input Buffer

Implement:

```cpp
class GameplayInputBuffer {
public:
    void PushInput(const GameplayInputFrame& input);
    bool GetInput(uint32_t playerIndex, GameplayInputFrame& outInput) const;
    void ClearForTick(uint64_t tick);
    void Reset();
};
```

Rules:

```text
Gameplay receives input per fixed tick.
Input is independent from SDL.
Input represents semantic gameplay actions, not mouse/key names.
Input sequence fields prepare for Phase 8 networking.
No networking code is implemented in Phase 7.
Client mapping follows `GAMEPLAY.md`: left click is contextual steal without puck and shot charge/release with puck, right click sets a waypoint, Z boosts, X quick-turns skaters or shields goalies, and S brakes/clears waypoint/double-tap stops.
```

## 6.4 Client Input Translation

`HockeyGameClient` may translate SDL/input system values into `GameplayInputFrame`.

This translation lives in app/client code, not in `hockey_gameplay`.

---

# 7. Gameplay Components

All gameplay components must be serializable where scene persistence makes sense and must have metadata for editor inspection where useful.

## 7.1 Player Components

Files:

```text
Player/PlayerComponents.hpp/cpp
```

Implement:

```cpp
struct PlayerComponent {
    uint32_t playerIndex = 0;
    PlayerSlot slot = PlayerSlot::None;
    GameplayTeam team = GameplayTeam::None;
    GameplayRole role = GameplayRole::Skater;

    bool controlledByLocalInput = false;
    bool controlledByAI = false;
    bool activeInMatch = true;
};

struct SkaterComponent {
    float maxSpeed = 9.0f;
    float acceleration = 32.0f;
    float deceleration = 20.0f;
    float turnSpeedDegrees = 720.0f;

    bool hasPuck = false;
};

struct GoalieComponent {
    float maxSpeed = 6.5f;
    float acceleration = 28.0f;
    float saveReachRadius = 1.8f;
    bool lockToCrease = true;
};

struct PlayerRuntimeComponent {
    glm::vec3 velocity{0.0f};
    glm::vec3 facingDirection{0.0f, 0.0f, 1.0f};
    glm::vec3 moveTarget{0.0f};

    float boostCooldown = 0.0f;
    uint32_t goalieBoostCharges = 2;
    float goalieBoostRechargeTimer = 0.0f;
    float shieldTimer = 0.0f;
    float shieldCooldown = 0.0f;
    float lastBrakePressedTime = -1000.0f;
    float checkCooldown = 0.0f;
    float pokeCheckCooldown = 0.0f;

    bool hasMoveTarget = false;
    bool shieldActive = false;
    bool inputEnabled = true;
    bool movementEnabled = true;
};
```

## 7.2 Puck Components

Files:

```text
Puck/PuckComponents.hpp/cpp
```

Implement:

```cpp
enum class PuckState {
    Loose,
    Possessed,
    Shot,
    Passed,
    Frozen,
    Resetting
};

struct PuckGameplayComponent {
    PuckState state = PuckState::Loose;
    UUID possessingPlayer;
    UUID lastTouchedPlayer;
    GameplayTeam lastTouchedTeam = GameplayTeam::None;

    float timeSinceLastTouch = 0.0f;
    bool inPlay = true;
};

struct PuckRuntimeComponent {
    glm::vec3 velocity{0.0f};
    glm::vec3 targetPosition{0.0f};
};
```

## 7.3 Stick Components

Files:

```text
Stick/StickComponents.hpp/cpp
```

Implement:

```cpp
struct StickComponent {
    UUID ownerPlayer;

    float reach = 1.5f;
    float width = 0.25f;
    glm::vec3 localOffset{0.0f, 0.0f, 1.0f};

    bool canControlPuck = true;
    bool canShoot = true;
    bool canPass = true;
    bool canCheck = true;
};

struct ShotComponent {
    float charge = 0.0f;
    bool charging = false;
};

struct PassComponent {
    UUID targetPlayer;
    float assistRadius = 2.0f;
};

struct CheckComponent {
    float cooldown = 0.0f;
};
```

## 7.4 Rink Gameplay Components

Files:

```text
Rink/RinkGameplayComponents.hpp/cpp
```

Implement:

```cpp
struct GoalGameplayComponent {
    GameplayTeam scoringTeam = GameplayTeam::None;
    GameplayTeam defendingTeam = GameplayTeam::None;
    bool enabled = true;
};

struct OutOfPlayComponent {
    glm::vec3 resetPosition{0.0f};
    float minY = -5.0f;
};

struct FaceoffGameplayComponent {
    int index = 0;
    bool centerIce = false;
    GameplayTeam preferredZone = GameplayTeam::None;
};
```

---

# 8. Serialization and Metadata

Update:

```text
ComponentSerializer
ComponentRegistry
SceneSerializer
SceneValidator
Inspector metadata
```

Register/serialize:

```text
MatchStateComponent
TeamStateComponent
PlayerComponent
SkaterComponent
GoalieComponent
PlayerRuntimeComponent where useful or runtime-only
PuckGameplayComponent
PuckRuntimeComponent where useful or runtime-only
StickComponent
ShotComponent
PassComponent
CheckComponent
GoalGameplayComponent
OutOfPlayComponent
FaceoffGameplayComponent
```

Rules:

```text
Authoring components serialize.
Runtime-only components may be transient.
Editor metadata should clearly mark runtime-only/read-only fields.
```

---

# 9. Gameplay World and System

## 9.1 GameplayWorld

Files:

```text
Simulation/GameplayWorld.hpp/cpp
```

Implement:

```cpp
class GameplayWorld {
public:
    Status Init(Scene& scene, PhysicsWorld* physicsWorld, const GameplaySettings& settings);
    void Shutdown();

    void InitializeMatch(Scene& scene);
    void ResetMatch(Scene& scene);

    void PushInput(const GameplayInputFrame& input);
    void FixedUpdate(Scene& scene, float fixedDeltaSeconds, uint64_t tick);

    std::vector<GameplayEvent> DrainEvents();

    const GameplaySettings& GetSettings() const;
    const GameplayTuning& GetTuning() const;

    GameplayInputBuffer& InputBuffer();
};
```

## 9.2 GameplaySystem

Files:

```text
Simulation/GameplaySystem.hpp/cpp
```

Integrate with Phase 2 `System`:

```cpp
class GameplaySystem final : public System {
public:
    GameplaySystem(PhysicsWorld* physicsWorld, GameplaySettings settings, GameplayTuning tuning);

    void OnStart(Scene& scene) override;
    void OnStop(Scene& scene) override;
    void OnFixedUpdate(Scene& scene, float fixedDeltaTime) override;

    GameplayWorld& World();

private:
    GameplayWorld m_World;
};
```

## 9.3 Fixed Update Order

Gameplay fixed tick order:

```text
1. Read input buffer for this tick.
2. Update match timers/phase timers.
3. Process match phase transitions.
4. Update player movement intent.
5. Update goalie movement intent.
6. Update stick positions/interactions.
7. Update puck possession rules.
8. Process shooting/pass/check actions.
9. Apply forces/velocities to physics bodies.
10. Step physics through PhysicsSystem or coordinate with scene physics step.
11. Read physics events.
12. Interpret goal trigger events.
13. Interpret out-of-play.
14. Update score/reset/faceoff if needed.
15. Emit gameplay events.
16. Produce gameplay snapshot data for future networking.
```

Coordinate with physics to avoid double stepping.

Preferred architecture:

```text
GameplaySystem computes desired body velocities/forces.
PhysicsSystem steps physics.
GameplaySystem consumes physics events after physics step.
```

If Scene system calls systems sequentially, define order explicitly.

---

# 10. Match Setup

## 10.1 Required Scene Data

Gameplay scene must contain:

```text
1 puck entity
2 goal entities
Home/Away spawn points:
  3 skater spawns per team
  1 goalie spawn per team
at least 1 faceoff spot
rink/play area markers
```

## 10.2 Match Initialization

Files:

```text
Match/MatchSystem.hpp/cpp
```

Implement:

```cpp
Status InitializeMatch(Scene& scene, GameplayWorld& world);
```

Behavior:

```text
validate scene
create MatchStateComponent on match state entity if missing
create TeamStateComponent entities if missing
find puck
find goals
find spawn points
find faceoff spots
spawn/create player entities if needed
assign players to slots
place players at spawn points
place puck at faceoff/puck spawn
first match start sets phase to Warmup for the 10-second countdown
goal/reset flow sets phase to FaceoffSetup after reset placement
emit MatchInitialized
```

## 10.3 Player Entity Creation

If scene has no player entities, create runtime player entities:

```text
Home Skater 0
Home Skater 1
Home Skater 2
Home Goalie
Away Skater 0
Away Skater 1
Away Skater 2
Away Goalie
```

Each player should have:

```text
PlayerComponent
SkaterComponent or GoalieComponent
PlayerRuntimeComponent
RigidBodyComponent
CapsuleColliderComponent
TransformComponent
optional MeshRendererComponent if renderer components exist
```

No final art requirement.

Use built-in fallback mesh/material names where possible.

## 10.4 Puck Entity Creation

If scene has no puck entity, create one.

Puck should have:

```text
PuckComponent marker
PuckGameplayComponent
PuckRuntimeComponent
RigidBodyComponent
SphereColliderComponent or CylinderColliderComponent
TransformComponent
optional MeshRendererComponent
```

---

# 11. Team and Role Assignment

## 11.1 TeamAssignment

Files:

```text
Teams/TeamAssignment.hpp/cpp
```

Implement:

```cpp
class TeamAssignment {
public:
    static Status AssignDefaultTeams(Scene& scene);
    static Status AssignPlayerToTeam(Scene& scene, UUID player, GameplayTeam team);
    static std::vector<UUID> GetPlayersOnTeam(Scene& scene, GameplayTeam team);
};
```

Rules:

```text
Home has exactly 4 assigned players.
Away has exactly 4 assigned players.
Each team has 3 skaters and 1 goalie.
No player can be assigned to both teams.
```

## 11.2 RoleAssignment

Files:

```text
Teams/RoleAssignment.hpp/cpp
```

Implement:

```cpp
class RoleAssignment {
public:
    static Status AssignRole(Scene& scene, UUID player, GameplayRole role);
    static Status AssignSlot(Scene& scene, UUID player, PlayerSlot slot);
    static bool IsSlotAvailable(Scene& scene, PlayerSlot slot);
};
```

Rules:

```text
slot unique
role matches slot
goalie slot gets GoalieComponent
skater slot gets SkaterComponent
```

---

# 12. Spawn System

Files:

```text
Player/PlayerSpawnSystem.hpp/cpp
```

Implement:

```cpp
struct SpawnAssignment {
    UUID player;
    UUID spawnPoint;
    PlayerSlot slot;
};

class PlayerSpawnSystem {
public:
    static Status AssignSpawns(Scene& scene, std::vector<SpawnAssignment>& outAssignments);
    static Status SpawnPlayers(Scene& scene, const std::vector<SpawnAssignment>& assignments);
    static Status RespawnPlayer(Scene& scene, UUID player);
};
```

Rules:

```text
Home skater slots use Home skater spawn points 0..2.
Home goalie uses Home goalie spawn 0.
Away skater slots use Away skater spawn points 0..2.
Away goalie uses Away goalie spawn 0.
Missing spawns produce validation errors.
Spawn preserves team/role identity.
```

---

# 13. Match Clock and Periods

## 13.1 MatchClock

Files:

```text
Match/MatchClock.hpp/cpp
```

Implement:

```cpp
class MatchClock {
public:
    static void Start(MatchStateComponent& match);
    static void Stop(MatchStateComponent& match);
    static void ResetPeriod(MatchStateComponent& match, float periodLengthSeconds);
    static void Tick(MatchStateComponent& match, float deltaSeconds);
};
```

## 13.2 PeriodSystem

Files:

```text
Match/PeriodSystem.hpp/cpp
```

Behavior:

```text
period starts at configured length
clock runs only during Playing
clock stops on goal if enabled
period ends when time reaches zero
period increments until periodCount
match ends after final period
emit PeriodStarted/PeriodEnded/MatchEnded events
```

---

# 14. Faceoff System

Files:

```text
Match/FaceoffSystem.hpp/cpp
```

Implement:

```cpp
class FaceoffSystem {
public:
    static Status StartFaceoff(Scene& scene, GameplayWorld& world, UUID faceoffSpot);
    static void UpdateFaceoff(Scene& scene, GameplayWorld& world, float deltaSeconds);
    static Status CompleteFaceoff(Scene& scene, GameplayWorld& world);
};
```

Behavior:

```text
move players to faceoff positions or nearby slots
move puck to faceoff spot
temporarily disable movement if needed
set puck state Frozen or Loose at completion
after faceoff delay, phase becomes Playing
emit FaceoffStarted and FaceoffEnded
```

No referee/drop animation required.

---

# 15. Skater Movement

Files:

```text
Player/PlayerMovement.hpp/cpp
Player/SkaterController.hpp/cpp
```

Gameplay should control physics bodies through desired velocity/forces.

Implement:

```cpp
class SkaterController {
public:
    static void UpdateSkater(
        Scene& scene,
        PhysicsWorld& physics,
        Entity player,
        const GameplayInputFrame& input,
        const GameplayTuning& tuning,
        float deltaSeconds
    );
};
```

Movement behavior:

```text
input move vector controls desired skating direction when no waypoint is active
right-click/setMoveTarget moves player toward a waypoint on the ice plane
reaching waypoint clears target
brakePressed clears current waypoint
double-tap brake within tuning window fully stops player
quickTurnPressed clears current waypoint
acceleration toward desired velocity
deceleration when no input
max speed clamp
boostPressed applies a short impulse in facing direction
boost cooldown prevents repeated impulse spam
turn/facing direction follows aim or movement
movement disabled during faceoff lock/reset
write velocity/force to physics body
```

Rules:

```text
No SDL input read here.
No renderer calls.
No animation calls.
```

---

# 16. Goalie Movement

Files:

```text
Player/GoalieController.hpp/cpp
```

Implement:

```cpp
class GoalieController {
public:
    static void UpdateGoalie(
        Scene& scene,
        PhysicsWorld& physics,
        Entity goalie,
        const GameplayInputFrame& input,
        const GameplayTuning& tuning,
        float deltaSeconds
    );
};
```

Behavior:

```text
goalie uses movement input
lower max speed than skater
can be constrained to crease if lockToCrease enabled
boostPressed consumes one of two goalie boost charges
goalie boost charges recharge one at a time every 4 seconds
goalieShieldPressed activates temporary shield if off cooldown
goalie shield reflects pucks and bounces players inside its sphere
goalie can block puck physically
```

Crease logic can use a configurable rectangular/circular region near goal.

No final goalie animation required.

---

# 17. Stick Handling and Puck Possession

## 17.1 Stick Handling

Files:

```text
Stick/StickHandling.hpp/cpp
```

Implement:

```cpp
class StickHandling {
public:
    static glm::vec3 ComputeStickWorldPosition(Scene& scene, Entity player);
    static bool IsPuckInControlRange(Scene& scene, Entity player, Entity puck, const GameplayTuning& tuning);
    static void UpdateStickControl(Scene& scene, Entity player, Entity puck, float deltaSeconds);
};
```

## 17.2 Puck Possession

Files:

```text
Puck/PuckPossession.hpp/cpp
```

Implement:

```cpp
class PuckPossession {
public:
    static bool CanPossess(Scene& scene, Entity player, Entity puck);
    static void GivePossession(Scene& scene, Entity player, Entity puck);
    static void ReleasePossession(Scene& scene, Entity puck);
    static Entity GetPossessingPlayer(Scene& scene, Entity puck);
};
```

Rules:

```text
Only one player can possess puck.
Puck possession follows stick/control radius.
Possession sets PuckGameplayComponent state to Possessed.
Possessed puck position follows player/stick offset.
Shot/pass/steal/check/goal/reset/out-of-play releases possession.
Loose puck can be touched/controlled.
```

## 17.3 Puck Interaction

Files:

```text
Puck/PuckInteraction.hpp/cpp
```

Handle:

```text
loose puck pickup
puck last touched player/team
puck deflection by player/goalie/stick
puck possession changes
```

Emit:

```text
PuckPossessionChanged
```

---

# 18. Shooting System

Files:

```text
Stick/ShootingSystem.hpp/cpp
```

Implement:

```cpp
class ShootingSystem {
public:
    static void UpdateShotCharging(Scene& scene, Entity player, const GameplayInputFrame& input, float deltaSeconds);
    static void TryShoot(Scene& scene, PhysicsWorld& physics, Entity player, Entity puck, const GameplayInputFrame& input, const GameplayTuning& tuning);
};
```

Behavior:

```text
shootPressed/shootHeld charges only while player has puck
shootReleased shoots only if player has puck
shot direction uses aim vector/facing direction
shot power depends on charge
shot releases possession
shot applies puck velocity/impulse
shot updates last touched player/team
emit PuckShot
```

Rules:

```text
shot power clamped
shot has deterministic/random-free default
accuracy can be deterministic based on tuning or disabled
```

No final animation/audio.

---

# 19. Passing System

Files:

```text
Stick/PassingSystem.hpp/cpp
```

Implement:

```cpp
class PassingSystem {
public:
    static Entity FindBestPassTarget(Scene& scene, Entity passer, glm::vec2 aim, const GameplayTuning& tuning);
    static void TryPass(Scene& scene, PhysicsWorld& physics, Entity player, Entity puck, const GameplayInputFrame& input, const GameplayTuning& tuning);
};
```

Behavior:

```text
passPressed attempts pass
requires possession or puck in stick range
find teammate target based on aim cone
assist target if within angle/radius
release possession
apply puck velocity toward target or aim direction
emit PuckPassed
```

No networking.

No UI pass target indicators required.

---

# 20. Checking System

Files:

```text
Stick/CheckingSystem.hpp/cpp
```

Implement:

```cpp
class CheckingSystem {
public:
    static void TryBodyCheck(Scene& scene, PhysicsWorld& physics, Entity player, const GameplayInputFrame& input, const GameplayTuning& tuning);
    static void TrySteal(Scene& scene, PhysicsWorld& physics, Entity player, Entity puck, const GameplayInputFrame& input, const GameplayTuning& tuning);
    static void TryPokeCheck(Scene& scene, PhysicsWorld& physics, Entity player, Entity puck, const GameplayInputFrame& input, const GameplayTuning& tuning);
};
```

Behavior:

```text
stealPressed/stealHeld attempts puck-first steal when player does not possess puck
steal uses stick/control volume in front of player
steal has cooldown even on failed attempts
steal can release opponent possession when in range
body check has cooldown
body check applies impulse/interaction to nearby opposing player
poke check has cooldown
poke check can knock puck loose if in stick range
checking disabled if settings disallow body checking
emit PlayerChecked
emit StealAttempted when useful for feedback/presentation
```

No penalties system yet unless trivially represented as future field.

---

# 21. Goal Detection and Score

## 21.1 Goal Detection

Files:

```text
Rink/GoalDetection.hpp/cpp
```

Implement:

```cpp
class GoalDetection {
public:
    static void ProcessPhysicsEvents(Scene& scene, GameplayWorld& world, const std::vector<PhysicsTriggerEvent>& triggerEvents);
    static bool IsGoalTrigger(Scene& scene, UUID entity);
    static GameplayTeam GetScoringTeamForGoal(Scene& scene, UUID goalEntity);
};
```

Behavior:

```text
goal is detected when puck enters enabled goal trigger
scoring team is opposite defending team
ignore repeated goal trigger events while phase is GoalScored/Resetting
emit GoalScored
```

## 21.2 ScoreSystem

Files:

```text
Match/ScoreSystem.hpp/cpp
```

Implement:

```cpp
class ScoreSystem {
public:
    static void AddGoal(Scene& scene, GameplayTeam scoringTeam);
    static uint32_t GetScore(Scene& scene, GameplayTeam team);
};
```

Behavior:

```text
increment score
update MatchStateComponent
update TeamStateComponent
emit ScoreChanged
set phase GoalScored
stop clock if configured
start post-goal reset timer
```

---

# 22. Reset and Out-of-Play

## 22.1 ResetSystem

Files:

```text
Match/ResetSystem.hpp/cpp
```

Implement:

```cpp
class ResetSystem {
public:
    static void BeginReset(Scene& scene, GameplayWorld& world);
    static void CompleteReset(Scene& scene, GameplayWorld& world);
};
```

Reset behavior:

```text
disable player inputs if needed
clear puck possession
reset puck velocity
move players to appropriate spawn/faceoff positions
move puck to faceoff spot or puck spawn
set phase FaceoffSetup or Faceoff
emit ResetStarted/ResetCompleted
```

## 22.2 OutOfPlaySystem

Files:

```text
Rink/OutOfPlaySystem.hpp/cpp
```

Implement:

```cpp
class OutOfPlaySystem {
public:
    static bool IsPuckOutOfPlay(Scene& scene, Entity puck, const GameplaySettings& settings);
    static void HandleOutOfPlay(Scene& scene, GameplayWorld& world);
};
```

Conditions:

```text
puck below minY
puck outside PlayAreaComponent bounds
puck stuck/invalid transform if needed
```

Behavior:

```text
emit PuckOutOfPlay
start reset/faceoff
```

---

# 23. Gameplay Snapshot

Files:

```text
Simulation/GameplaySnapshot.hpp/cpp
```

This is not networking, but prepares Phase 8.

Implement pure data snapshot:

```cpp
struct PlayerGameplaySnapshot {
    UUID entity;
    uint32_t playerIndex;
    GameplayTeam team;
    GameplayRole role;
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 facingDirection;
    bool hasPuck = false;
};

struct PuckGameplaySnapshot {
    UUID entity;
    PuckState state;
    glm::vec3 position;
    glm::vec3 velocity;
    UUID possessingPlayer;
};

struct MatchGameplaySnapshot {
    MatchPhase phase;
    uint32_t period;
    float periodTimeRemaining;
    uint32_t homeScore;
    uint32_t awayScore;
};

struct GameplaySnapshot {
    uint64_t tick = 0;
    MatchGameplaySnapshot match;
    std::vector<PlayerGameplaySnapshot> players;
    PuckGameplaySnapshot puck;
};
```

Implement:

```cpp
GameplaySnapshot BuildGameplaySnapshot(Scene& scene, uint64_t tick);
```

Rules:

```text
No packet serialization.
No compression.
No network transport.
Pure data only.
```

---

# 24. Gameplay Validation

Files:

```text
Validation/GameplayValidation.hpp/cpp
Rink/RinkValidation.hpp/cpp
```

Implement validation issues for:

```text
missing puck
multiple pucks
missing home goal
missing away goal
more than two goals
invalid goal defending team
missing play area
missing rink marker
missing center faceoff
missing home skater spawn 0/1/2
missing home goalie spawn
missing away skater spawn 0/1/2
missing away goalie spawn
duplicate spawn index
invalid spawn team
invalid spawn role
player without team
player without role
too many players on team
too many goalies
puck missing physics body/collider
goal trigger missing trigger collider
player missing rigid body/capsule
goalie missing rigid body/capsule
skater-only collider has player/skater filtering
goalie-only collider has goalie filtering
goal trigger detects puck and ignores non-puck scoring
```

Integrate with:

```text
SceneValidationPanel
editor validate action
server scene load validation
gameplay tests
```

Errors should block match initialization.

Warnings can be shown in editor.

---

# 25. Editor Integration

Update editor to support gameplay authoring/preview.

## 25.1 Inspector

Add metadata/editors for gameplay components:

```text
PlayerComponent
SkaterComponent
GoalieComponent
PuckGameplayComponent
StickComponent
GoalGameplayComponent
FaceoffGameplayComponent
OutOfPlayComponent
MatchStateComponent
```

Runtime fields should be read-only where appropriate.

## 25.2 Scene Validation

Editor Scene Validation Panel should include gameplay validation.

## 25.3 Hockey Tools Update

Update hockey tools so generated objects include full gameplay-ready components:

```text
Puck tool:
  PuckComponent
  PuckGameplayComponent
  PuckRuntimeComponent
  RigidBodyComponent
  collider
  MeshRendererComponent if available

Goal tool:
  GoalComponent
  GoalGameplayComponent
  TriggerComponent
  trigger collider

Spawn tool:
  SpawnPointComponent
  TeamComponent
  PlayerRoleComponent

Faceoff tool:
  FaceoffSpotComponent
  FaceoffGameplayComponent

Rink tool:
  RinkComponent
  PlayAreaComponent
  static colliders where appropriate
```

## 25.4 Gameplay Preview

Add editor preview controls if not already present:

```text
Initialize Match
Start/Stop Gameplay Preview
Step Gameplay Tick
Reset Match
Show Gameplay Debug
```

Preview must not permanently mutate authoring scene unless user applies changes.

Use a duplicate preview scene or provide reset behavior.

---

# 26. Client Integration

Update `HockeyGameClient`.

Required behavior:

```text
load gameplay settings
load gameplay tuning
initialize physics
initialize gameplay
load scene
initialize local match
create local input frames from existing input system
feed input to GameplayWorld
run fixed gameplay/physics updates
render resulting scene
log gameplay events when enabled
allow restart/reset local match through temporary key
```

No final game UI is required.

Minimum temporary controls:

```text
WASD/left stick direct movement fallback
mouse/right stick aim
right click set waypoint
left click contextual steal or shot charge/release
pass button/key
Z boost
X skater slide stop / goalie shield
S brake, clear waypoint, double-tap full stop
reset match key for development
```

The input mapping can live in client app code and should be simple.

---

# 27. Dedicated Server Integration

Update `HockeyDedicatedServer`.

Required behavior:

```text
load gameplay settings
load gameplay tuning
load scene
validate gameplay scene
initialize physics
initialize gameplay
initialize match
run fixed tick authoritative simulation
feed deterministic test inputs or no input if no clients
drain/log gameplay events
allow command-line option to run local bot/test inputs if useful
shutdown cleanly
remain headless
```

No networking.

No lobbies.

The server should prove gameplay can run without renderer/editor/UI.

---

# 28. Map Editor Integration

Update `HockeyMapEditor`.

Required behavior:

```text
load gameplay settings/tuning
validate gameplay scene
show gameplay validation issues
support gameplay preview
support hockey tools generating gameplay-ready entities
allow editing gameplay tuning file if simple text/project panel support exists
```

No final game UI.

---

# 29. Gameplay Tests

Create:

```text
HockeyGameplayTests
```

Use simple test harness.

## 29.1 MatchSetupTests

Test:

```text
valid rink scene initializes match
missing puck fails match init
missing goals fails match init
missing spawns fails match init
match state created
team state created
puck found/created
players found/created
```

## 29.2 TeamAssignmentTests

Test:

```text
default teams assign 4 home and 4 away
home has 3 skaters and 1 goalie
away has 3 skaters and 1 goalie
no duplicate team assignment
opposite team helper works
```

## 29.3 RoleAssignmentTests

Test:

```text
slot assignment works
slot cannot be duplicated
goalie slot gets goalie role
skater slot gets skater role
invalid slot rejected
```

## 29.4 SpawnSystemTests

Test:

```text
home skaters spawn at home skater spawns
home goalie spawns at home goalie spawn
away skaters spawn at away skater spawns
away goalie spawns at away goalie spawn
missing spawn fails validation
respawn player works
```

## 29.5 InputModelTests

Test:

```text
input frame stores move/aim/actions
input frame stores move target set/clear actions
input frame stores semantic steal/boost/brake/quick-turn/goalie-shield actions
input buffer stores per-player input
latest input retrieved
clear for tick works
sequence/tick fields preserved
```

## 29.6 SkaterMovementTests

Test:

```text
movement input accelerates skater
waypoint target moves skater toward target
brake clears waypoint
double-tap brake fully stops skater
no input decelerates skater
speed clamps to max speed
boost applies impulse and respects cooldown
quick turn clears waypoint and damps velocity
facing direction updates
movement disabled blocks movement
```

## 29.7 GoalieMovementTests

Test:

```text
goalie movement works
goalie max speed lower than skater
crease lock constrains movement
goalie starts with two boost charges
goalie boost consumes one charge
goalie boost recharges one charge every 4 seconds
goalie shield starts, expires, and respects cooldown
goalie shield reflects puck
goalie shield bounces players
```

## 29.8 PuckInteractionTests

Test:

```text
loose puck can be possessed
only one player can possess puck
possession follows player/stick offset
release possession makes puck loose
last touched player/team update
```

## 29.9 StickHandlingTests

Test:

```text
stick world position computed
puck in control range detected
puck outside range ignored
stick owner mapping works
```

## 29.10 ShootingTests

Test:

```text
shoot charge increases while held
shoot release applies puck velocity
shoot releases possession
left click with possession maps to charge/release shot in client input
shot direction follows aim
shot power clamps
PuckShot event emitted
```

## 29.11 PassingTests

Test:

```text
pass finds teammate target
pass ignores opponent
pass uses aim cone
pass applies puck velocity
PuckPassed event emitted
```

## 29.12 CheckingTests

Test:

```text
body check cooldown works
body check affects nearby opponent
left click without possession attempts steal in client input
steal releases opponent possession when in range
steal cooldown applies on failed attempts
poke check can release puck
checking disabled setting prevents body check
PlayerChecked event emitted
StealAttempted event emitted when useful
```

## 29.13 GoalDetectionTests

Test:

```text
puck entering home defending goal scores away
puck entering away defending goal scores home
non-puck entering goal does not score
repeated trigger while GoalScored ignored
GoalScored event emitted
```

## 29.14 ScoreTests

Test:

```text
home score increments
away score increments
MatchState score updates
TeamState score updates
ScoreChanged event emitted
```

## 29.15 FaceoffTests

Test:

```text
faceoff starts
players positioned
puck positioned
movement lock applied if enabled
faceoff completes
phase becomes Playing
events emitted
```

## 29.16 PeriodClockTests

Test:

```text
clock starts in Playing
clock does not run in Faceoff
pregame countdown starts at 10 seconds
pregame countdown locks player movement and puck interaction
pregame countdown emits tick events every second
pregame countdown emits beep events for the last 4 seconds
pregame countdown transitions to faceoff setup
clock stops after goal if enabled
period starts at 180 seconds
period ends at zero
match ends after final period
```

## 29.17 ResetSystemTests

Test:

```text
goal starts reset
reset clears puck possession
reset moves puck
reset respawns players
reset transitions to faceoff
events emitted
```

## 29.18 GameplayValidationTests

Test:

```text
valid scene passes
missing puck error
multiple pucks error
missing goals error
missing spawn error
invalid role/team error
goal trigger missing warning/error
puck physics setup invalid warning/error
skater-only collider authoring works
goalie-only collider authoring works
goal trigger scores only puck
```

## 29.19 GameplaySnapshotTests

Test:

```text
snapshot includes tick
snapshot includes match state
snapshot includes 8 players
snapshot includes puck
snapshot updates after movement/goal
snapshot has no networking dependency
```

## 29.20 HeadlessServerGameplayTests

Test:

```text
server scene initializes gameplay
server fixed tick updates gameplay
server can simulate several seconds
server detects goal
server remains headless
server does not require renderer/editor
```

---

# 30. Implementation Order for AI Agents

Implement in this exact order.

## Step 1 — Target Setup

```text
1. Create libs/gameplay folder.
2. Create libs/gameplay/CMakeLists.txt.
3. Add hockey_gameplay target.
4. Create apps/gameplay_tests target.
5. Update root CMakeLists.txt.
6. Link client/editor/server/gameplay_tests to hockey_gameplay.
7. Confirm hockey_gameplay does not link renderer/editor/networking/SDL/Vulkan/ImGui.
8. Configure/build.
```

## Step 2 — Settings, Tuning, Core Types

```text
1. Add GameplaySettings.
2. Add config load/save.
3. Add GameplayTypes.
4. Add TeamTypes.
5. Add GameplayTuning.
6. Add TuningSerializer.
7. Add data/gameplay YAML files.
8. Add basic tests for settings/tuning.
```

## Step 3 — Input and Events

```text
1. Add GameplayInputFrame.
2. Add GameplayInputBuffer.
3. Add GameplayEvents.
4. Add event queue/drain support.
5. Add InputModelTests.
```

## Step 4 — Components

```text
1. Add MatchStateComponent.
2. Add TeamStateComponent.
3. Add PlayerComponent.
4. Add SkaterComponent.
5. Add GoalieComponent.
6. Add PlayerRuntimeComponent.
7. Add PuckGameplayComponent.
8. Add PuckRuntimeComponent.
9. Add StickComponent.
10. Add ShotComponent.
11. Add PassComponent.
12. Add CheckComponent.
13. Add GoalGameplayComponent.
14. Add FaceoffGameplayComponent.
15. Add OutOfPlayComponent.
16. Add serialization/metadata.
17. Build and test.
```

## Step 5 — Validation and Match Setup

```text
1. Add GameplayValidation.
2. Add RinkValidation.
3. Implement required scene checks.
4. Add MatchSystem.
5. Implement InitializeMatch.
6. Add TeamAssignment.
7. Add RoleAssignment.
8. Add PlayerSpawnSystem.
9. Add MatchSetup/Team/Role/Spawn tests.
```

## Step 6 — GameplayWorld/System

```text
1. Add GameplayWorld.
2. Add GameplaySystem.
3. Connect physics world pointer.
4. Implement FixedUpdate order.
5. Add basic headless update test.
```

## Step 7 — Match Clock, Period, Faceoff, Reset

```text
1. Add MatchClock.
2. Add PeriodSystem.
3. Add pregame countdown state/events.
4. Add FaceoffSystem.
5. Add ResetSystem.
6. Add PeriodClockTests, including 180-second period defaults.
7. Add pregame countdown tests.
8. Add FaceoffTests.
9. Add ResetSystemTests.
```

## Step 8 — Player and Goalie Movement / Abilities

```text
1. Add PlayerMovement.
2. Add SkaterController.
3. Add GoalieController.
4. Apply movement to physics bodies.
5. Add waypoint set/clear behavior.
6. Add skater boost impulse and cooldown.
7. Add S double-tap stop.
8. Add goalie boost charges and recharge.
9. Add goalie shield state, reflection, and bounce response.
10. Add SkaterMovementTests.
11. Add GoalieMovementTests.
```

## Step 9 — Puck, Stick, Shot, Pass, Check

```text
1. Add PuckPossession.
2. Add PuckInteraction.
3. Add StickHandling.
4. Add ShootingSystem.
5. Add PassingSystem.
6. Add CheckingSystem with explicit steal action.
7. Add PuckInteractionTests.
8. Add StickHandlingTests.
9. Add ShootingTests.
10. Add PassingTests.
11. Add CheckingTests and StealTests if split out.
```

## Step 10 — Goal, Score, Out-of-Play

```text
1. Add GoalDetection.
2. Add ScoreSystem.
3. Add OutOfPlaySystem.
4. Add PuckResetSystem if separate from ResetSystem.
5. Add GoalDetectionTests.
6. Add ScoreTests.
```

## Step 11 — Snapshot

```text
1. Add GameplaySnapshot.
2. Implement BuildGameplaySnapshot.
3. Add GameplaySnapshotTests.
```

## Step 12 — Editor Integration

```text
1. Update editor component metadata/inspector support.
2. Update scene validation panel to include gameplay validation.
3. Update hockey tools to create gameplay-ready entities.
4. Add gameplay preview controls.
5. Verify editor preview does not permanently corrupt authoring scene.
```

## Step 13 — Client Integration

```text
1. Load gameplay settings/tuning.
2. Initialize gameplay and physics.
3. Initialize local match.
4. Translate local input to GameplayInputFrame in client code.
5. Map local controls according to `GAMEPLAY.md` contextual action rules.
6. Feed input to GameplayWorld.
7. Run fixed gameplay simulation.
8. Render resulting scene.
9. Add temporary reset/development controls.
```

## Step 14 — Server Integration

```text
1. Load gameplay settings/tuning.
2. Load scene.
3. Validate gameplay scene.
4. Initialize physics/gameplay.
5. Initialize match.
6. Run fixed tick simulation.
7. Drain/log gameplay events.
8. Remain headless.
9. Add HeadlessServerGameplayTests.
```

## Step 15 — Final Verification

```text
1. Build Debug Windows.
2. Build Release Windows.
3. Build Debug Linux.
4. Build Release Linux.
5. Run all previous tests.
6. Run HockeyGameplayTests.
7. Run client local gameplay.
8. Run editor gameplay validation/preview.
9. Run dedicated server headless gameplay simulation.
10. Confirm no networking/gameplay netcode was added.
```

---

# 31. Verification Commands

## Windows

```powershell
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\build\windows-debug\apps\core_tests\HockeyCoreTests.exe --root .
.\build\windows-debug\apps\ecs_tests\HockeyECSTests.exe --root .
.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root .
.\build\windows-debug\apps\editor_tests\HockeyEditorTests.exe --root .
.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .
.\build\windows-debug\apps\physics_tests\HockeyPhysicsTests.exe --root .
.\build\windows-debug\apps\gameplay_tests\HockeyGameplayTests.exe --root .
.\scripts\windows\run_client.ps1
.\scripts\windows\run_editor.ps1
.\scripts\windows\run_server.ps1
```

## Linux

```bash
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./build/linux-debug/apps/core_tests/HockeyCoreTests --root .
./build/linux-debug/apps/ecs_tests/HockeyECSTests --root .
./build/linux-debug/apps/renderer_tests/HockeyRendererTests --root .
./build/linux-debug/apps/editor_tests/HockeyEditorTests --root .
./build/linux-debug/apps/asset_tests/HockeyAssetTests --root .
./build/linux-debug/apps/physics_tests/HockeyPhysicsTests --root .
./build/linux-debug/apps/gameplay_tests/HockeyGameplayTests --root .
./scripts/linux/run_client.sh
./scripts/linux/run_editor.sh
./scripts/linux/run_server.sh
```

---

# 32. Phase 7 Definition of Done

Phase 7 is complete only when all of this is true:

```text
hockey_gameplay builds
HockeyGameplayTests builds
HockeyGameplayTests pass
GameplaySettings load from config
GameplayTuning loads from YAML
GameplayInputFrame works
GameplayInputBuffer works
gameplay events work
MatchStateComponent serializes
TeamStateComponent serializes
PlayerComponent serializes
SkaterComponent serializes
GoalieComponent serializes
PuckGameplayComponent serializes
StickComponent serializes
GoalGameplayComponent serializes
FaceoffGameplayComponent serializes
gameplay metadata registered
gameplay validation works
valid rink scene initializes match
4v4 player setup works
3 skaters + 1 goalie per team works
team assignment works
role assignment works
spawn assignment works
puck setup works
faceoff works
period clock works
3x180-second period defaults work
pregame countdown works
countdown tick/beep gameplay events work
score works
goal detection works
reset flow works
out-of-play handling works
skater movement works
skater boost impulse works
S clears waypoint and double-tap stops
goalie movement works
goalie two-charge boost works
goalie shield reflects puck and bounces players
stick control works
puck possession works
contextual left-click steal-or-shot behavior works
shooting works
passing works
checking/poke check hooks work
role-specific skater-only and goalie-only collider authoring works
goal trigger scoring is puck-only
gameplay snapshots work
client can run local gameplay
editor can validate gameplay scene
editor can preview gameplay
server can run gameplay headlessly
server does not require renderer/editor/UI
Windows build works
Linux build works
no GameNetworkingSockets code exists
no online lobby code exists
no replication protocol exists
no final animation/audio/UI exists
```

---

# 33. AI Completion Instruction

When Phase 7 is complete, AI agent should report:

```text
Phase 7 complete.

Implemented:
- hockey_gameplay
- HockeyGameplayTests
- gameplay settings/config
- gameplay tuning YAML
- gameplay input model
- gameplay event queue
- match state and match system
- team/role/slot assignment
- player spawn system
- 4v4 setup
- skater movement
- goalie movement
- puck gameplay state
- stick handling
- puck possession
- shooting
- passing
- checking/poke check hooks
- goal detection
- score system
- period clock
- faceoff system
- reset system
- out-of-play handling
- gameplay validation
- gameplay snapshots
- editor gameplay validation/preview integration
- client local gameplay integration
- server headless gameplay integration

Verified:
- gameplay tests
- client local simulation
- editor gameplay validation/preview
- server headless simulation
- Windows/Linux builds
- no future-phase networking/animation/audio/UI systems were implemented
```
