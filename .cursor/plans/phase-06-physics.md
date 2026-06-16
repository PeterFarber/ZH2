# Phase 6 Plan — Complete Physics System

File path recommendation:

```text
.cursor/plans/phase-06-physics.md
```

Cursor instruction:

```text
Read `.cursor/plans/phase-06-physics.md` and implement Phase 6 exactly. Build the complete physics system using Jolt Physics, including ECS physics components, physics world simulation, collision layers, rigid bodies, colliders, triggers, queries, debug drawing, editor integration, and server-safe fixed-tick simulation. Do not implement networking, GameNetworkingSockets, hockey gameplay rules, final animation, audio, or final game UI during this phase. Keep the dedicated server headless and keep Windows/Linux compatibility.
```

---

# 0. Phase Summary

Phase 6 builds the complete physics system.

This phase adds a shared physics library used by:

```text
HockeyGameClient
HockeyMapEditor
HockeyDedicatedServer
```

The physics system must support:

```text
authoritative server simulation
editor physics preview
client-side local/prediction-ready simulation path
collision events
trigger events
raycasts
shape casts if feasible
overlap queries
debug drawing
physics materials
collision layers
hockey-specific physics setup data
```

Phase 6 must produce:

```text
hockey_physics
HockeyPhysicsTests
```

Phase 6 updates:

```text
hockey_ecs
hockey_editor
hockey_renderer debug draw usage
HockeyGameClient
HockeyMapEditor
HockeyDedicatedServer
data/config
data/raw/scenes
```

Phase 6 must not implement:

```text
networking
GameNetworkingSockets
lobbies
replication
hockey gameplay rules
match scoring
shooting/passing/checking gameplay logic
final animation
audio
final game UI
```

This phase gives later gameplay and networking phases a complete, stable physics foundation.

---

# 1. Architecture Rules

## 1.1 Dependency Direction

Add:

```text
hockey_physics -> hockey_core, hockey_ecs
```

Allowed app links:

```text
HockeyGameClient       -> hockey_physics
HockeyMapEditor        -> hockey_physics
HockeyDedicatedServer  -> hockey_physics
HockeyPhysicsTests     -> hockey_physics
```

Allowed editor link:

```text
hockey_editor -> hockey_physics
```

This allows the editor to draw/edit colliders and run preview simulation.

Renderer relationship:

```text
hockey_physics must not depend on hockey_renderer.
hockey_renderer must not depend on hockey_physics.
```

Debug drawing should be done through an abstract physics debug draw interface that the app/editor can bridge to renderer debug draw.

## 1.2 Physics Owns

`hockey_physics` owns:

```text
Jolt initialization/shutdown
physics world
body creation/destruction
collider/shape creation
physics materials
collision layers
collision filtering
simulation stepping
fixed tick simulation
body transform sync
raycasts
shape casts if implemented
overlap queries
contact events
trigger events
physics debug draw data
physics scene validation
```

## 1.3 Physics Must Not Own

`hockey_physics` must not own:

```text
hockey rules
goals/scoring
player input interpretation
network packets
replication
renderer GPU resources
editor UI panels
asset import/cooking
animation state
audio playback
```

## 1.4 Server Rule

The dedicated server must:

```text
link hockey_physics
run physics headlessly
not link renderer
not link editor
not create a window
not require GPU/display
```

---

# 2. Dependencies

Update `vcpkg.json`.

Add:

```json
"joltphysics"
```

If the exact vcpkg port name differs, use the project’s available port and document the target name in CMake.

Do not add:

```text
GameNetworkingSockets
FMOD/Wwise
animation middleware
extra editor UI dependencies
```

---

# 3. Target Project Structure

Add:

```text
libs/
  physics/
    CMakeLists.txt
    include/Hockey/Physics/
      Physics.hpp
      PhysicsSettings.hpp
      PhysicsWorld.hpp
      PhysicsScene.hpp
      PhysicsSystem.hpp
      PhysicsComponents.hpp
      PhysicsMaterial.hpp
      PhysicsLayer.hpp
      PhysicsBody.hpp
      PhysicsShape.hpp
      PhysicsEvents.hpp
      PhysicsQueries.hpp
      PhysicsDebugDraw.hpp
      PhysicsValidation.hpp
      Jolt/
        JoltCommon.hpp
        JoltTypeConversions.hpp
        JoltPhysicsWorld.hpp
        JoltBodyInterface.hpp
        JoltShapeFactory.hpp
        JoltContactListener.hpp
        JoltBroadPhaseLayer.hpp
        JoltDebugRenderer.hpp
    src/
      Physics.cpp
      PhysicsSettings.cpp
      PhysicsWorld.cpp
      PhysicsScene.cpp
      PhysicsSystem.cpp
      PhysicsComponents.cpp
      PhysicsMaterial.cpp
      PhysicsLayer.cpp
      PhysicsBody.cpp
      PhysicsShape.cpp
      PhysicsEvents.cpp
      PhysicsQueries.cpp
      PhysicsDebugDraw.cpp
      PhysicsValidation.cpp
      Jolt/
        JoltCommon.cpp
        JoltTypeConversions.cpp
        JoltPhysicsWorld.cpp
        JoltBodyInterface.cpp
        JoltShapeFactory.cpp
        JoltContactListener.cpp
        JoltBroadPhaseLayer.cpp
        JoltDebugRenderer.cpp
apps/
  physics_tests/
    CMakeLists.txt
    src/
      PhysicsTestsMain.cpp
      Test.hpp
      PhysicsInitTests.cpp
      PhysicsComponentTests.cpp
      PhysicsWorldTests.cpp
      RigidBodyTests.cpp
      ColliderTests.cpp
      PhysicsMaterialTests.cpp
      CollisionLayerTests.cpp
      QueryTests.cpp
      TriggerTests.cpp
      ContactEventTests.cpp
      SceneSyncTests.cpp
      HockeyPhysicsSetupTests.cpp
      HeadlessServerPhysicsTests.cpp
```

Update ECS serialization/metadata for physics components.

Update editor panels/tools to support physics components.

---

# 4. CMake Requirements

## 4.1 Root CMake

Add:

```cmake
add_subdirectory(libs/physics)
```

When tests are enabled:

```cmake
add_subdirectory(apps/physics_tests)
```

Ordering:

```text
core
ecs
assets
renderer
physics
editor
apps
```

`hockey_physics` must be built before apps that link it.

## 4.2 `libs/physics/CMakeLists.txt`

Create:

```text
hockey_physics
```

Required links:

```text
hockey_core
hockey_ecs
Jolt target from vcpkg
```

Do not link:

```text
hockey_renderer
hockey_editor
hockey_networking
hockey_gameplay
```

## 4.3 App Link Updates

Update:

```text
HockeyGameClient links hockey_physics
HockeyMapEditor links hockey_physics
HockeyDedicatedServer links hockey_physics
HockeyPhysicsTests links hockey_physics
```

`HockeyDedicatedServer` still must not link renderer/editor/ImGui.

## 4.4 Editor Link Update

Update:

```text
hockey_editor links hockey_physics
```

Only for:

```text
physics component inspector support
collider visualization
physics preview controls
scene validation
```

---

# 5. Physics Settings

## 5.1 Files

```text
PhysicsSettings.hpp
PhysicsSettings.cpp
```

## 5.2 Settings Struct

Implement:

```cpp
struct PhysicsSettings {
    float fixedDeltaSeconds = 1.0f / 60.0f;

    uint32_t maxBodies = 65536;
    uint32_t maxBodyPairs = 65536;
    uint32_t maxContactConstraints = 20000;

    uint32_t tempAllocatorSizeMB = 64;
    uint32_t jobThreadCount = 0;

    glm::vec3 gravity{0.0f, -9.81f, 0.0f};

    uint32_t collisionSteps = 1;
    uint32_t integrationSubsteps = 1;

    bool deterministicMode = false;
    bool enableSleeping = true;
    bool enableDebugDraw = false;

    float worldMinY = -1000.0f;
};
```

## 5.3 Config

Update config files.

`client.toml`:

```toml
[physics]
fixed_delta_seconds = 0.0166666667
gravity = [0.0, -9.81, 0.0]
enable_debug_draw = false
enable_sleeping = true
```

`editor.toml`:

```toml
[physics]
fixed_delta_seconds = 0.0166666667
gravity = [0.0, -9.81, 0.0]
enable_debug_draw = true
enable_sleeping = true
preview_simulation = false
```

`server.toml`:

```toml
[physics]
fixed_delta_seconds = 0.0166666667
gravity = [0.0, -9.81, 0.0]
enable_debug_draw = false
enable_sleeping = true
deterministic_mode = true
```

## 5.4 Required Functions

Implement:

```cpp
PhysicsSettings MakeDefaultPhysicsSettings();
Status LoadPhysicsSettings(Config& config, PhysicsSettings& outSettings);
void SavePhysicsSettings(Config& config, const PhysicsSettings& settings);
```

---

# 6. Physics Initialization

## 6.1 Files

```text
Physics.hpp/cpp
JoltCommon.hpp/cpp
```

## 6.2 API

Implement:

```cpp
class Physics {
public:
    static Status Init();
    static void Shutdown();
    static bool IsInitialized();
};
```

## 6.3 Jolt Requirements

Initialize:

```text
JPH::RegisterDefaultAllocator
JPH::Factory
JPH::RegisterTypes
trace/assert callbacks if desired
```

Shutdown:

```text
JPH::UnregisterTypes
destroy factory
```

Rules:

```text
Init may be called once.
Shutdown must be safe.
No physics world should exist after Shutdown.
Log initialization status.
```

---

# 7. Physics Layers and Filtering

## 7.1 Files

```text
PhysicsLayer.hpp/cpp
JoltBroadPhaseLayer.hpp/cpp
```

## 7.2 Object Layers

Implement:

```cpp
enum class PhysicsLayer : uint16_t {
    Static = 0,
    Player = 1,
    Goalie = 2,
    Puck = 3,
    Stick = 4,
    Rink = 5,
    Goal = 6,
    Trigger = 7,
    Sensor = 8,
    Editor = 9
};
```

## 7.3 Collision Matrix

Implement default collision rules:

```text
Static collides with Player, Goalie, Puck, Stick
Player collides with Static, Player, Goalie, Puck, Rink, Goal, Trigger
Goalie collides with Static, Player, Goalie, Puck, Rink, Goal, Trigger
Puck collides with Static, Player, Goalie, Puck, Stick, Rink, Goal, Trigger
Stick collides with Puck, Player, Goalie if enabled
Rink collides with Player, Goalie, Puck, Stick
Goal collides with Player, Goalie, Puck
Trigger collides with Player, Goalie, Puck
Sensor collides with selected query layers
Editor collides with nothing by default
```

## 7.4 API

Implement:

```cpp
class CollisionFilter {
public:
    static bool ShouldCollide(PhysicsLayer a, PhysicsLayer b);
    static void SetShouldCollide(PhysicsLayer a, PhysicsLayer b, bool shouldCollide);
    static void ResetDefaults();
};
```

Jolt broadphase layer mapping must support these layers.

---

# 8. Physics Materials

## 8.1 Files

```text
PhysicsMaterial.hpp/cpp
```

## 8.2 Struct

Implement:

```cpp
struct PhysicsMaterial {
    std::string name = "Default";

    float friction = 0.5f;
    float restitution = 0.1f;

    float linearDamping = 0.0f;
    float angularDamping = 0.0f;

    bool combineFrictionMultiply = false;
    bool combineRestitutionMax = false;
};
```

## 8.3 Built-In Materials

Implement built-ins:

```text
Default
Ice
PuckRubber
PlayerBody
GoalieBody
Boards
Glass
GoalNet
Stick
Trigger
```

Suggested values:

```text
Ice: low friction, low damping
PuckRubber: low/medium friction, high restitution
Boards: medium friction, medium/high restitution
Glass: low/medium friction, medium restitution
GoalNet: high damping, low restitution
Trigger: no physical response
```

Do not tune final gameplay values here. Provide stable defaults and make them editable.

---

# 9. Physics ECS Components

## 9.1 Files

```text
PhysicsComponents.hpp/cpp
```

Preferred dependency model:

```text
Physics components live in hockey_physics.
hockey_physics registers serializers/metadata through explicit registration function.
```

## 9.2 RigidBodyComponent

Implement:

```cpp
enum class RigidBodyType {
    Static,
    Kinematic,
    Dynamic
};

struct RigidBodyComponent {
    RigidBodyType type = RigidBodyType::Static;

    float mass = 1.0f;
    bool useGravity = true;
    bool allowSleeping = true;

    bool lockTranslationX = false;
    bool lockTranslationY = false;
    bool lockTranslationZ = false;

    bool lockRotationX = false;
    bool lockRotationY = false;
    bool lockRotationZ = false;

    float linearDamping = 0.0f;
    float angularDamping = 0.05f;

    glm::vec3 initialLinearVelocity{0.0f};
    glm::vec3 initialAngularVelocity{0.0f};

    PhysicsLayer layer = PhysicsLayer::Static;
    std::string materialName = "Default";
};
```

## 9.3 Collider Components

Implement:

```cpp
struct BoxColliderComponent {
    glm::vec3 halfExtents{0.5f};
    glm::vec3 offset{0.0f};
    glm::vec3 rotation{0.0f};
    bool isTrigger = false;
};

struct SphereColliderComponent {
    float radius = 0.5f;
    glm::vec3 offset{0.0f};
    bool isTrigger = false;
};

struct CapsuleColliderComponent {
    float radius = 0.5f;
    float halfHeight = 1.0f;
    glm::vec3 offset{0.0f};
    glm::vec3 rotation{0.0f};
    bool isTrigger = false;
};

struct CylinderColliderComponent {
    float radius = 0.5f;
    float halfHeight = 0.5f;
    glm::vec3 offset{0.0f};
    glm::vec3 rotation{0.0f};
    bool isTrigger = false;
};

struct MeshColliderComponent {
    AssetID meshAsset;
    bool convex = false;
    bool isTrigger = false;
};
```

## 9.4 Trigger Component

Implement:

```cpp
struct TriggerComponent {
    std::string tag;
    bool detectPlayers = true;
    bool detectGoalies = true;
    bool detectPuck = true;
};
```

## 9.5 Character Controller Component

If feasible, implement:

```cpp
struct CharacterControllerComponent {
    float radius = 0.45f;
    float height = 1.8f;
    float stepHeight = 0.25f;
    float maxSlopeDegrees = 45.0f;
};
```

If Jolt character controller integration is too large, create data component and validation only, but do not claim controller runtime is complete unless it works.

## 9.6 Serialization and Metadata

Update:

```text
ComponentSerializer
ComponentRegistry
SceneSerializer
SceneValidator
Inspector field drawers
```

All physics components must be serializable and editable.

---

# 10. Physics Body and Shape Abstractions

## 10.1 PhysicsBody

Files:

```text
PhysicsBody.hpp/cpp
JoltBodyInterface.hpp/cpp
```

Implement handle:

```cpp
struct PhysicsBodyHandle {
    uint32_t id = 0;
    bool IsValid() const { return id != 0; }
};
```

Mapping:

```text
Entity UUID -> PhysicsBodyHandle
PhysicsBodyHandle -> Jolt BodyID
Jolt BodyID -> Entity UUID
```

## 10.2 PhysicsShape

Files:

```text
PhysicsShape.hpp/cpp
JoltShapeFactory.hpp/cpp
```

Implement shape creation for:

```text
box
sphere
capsule
cylinder
convex mesh
static triangle mesh
compound shape
trigger shapes
```

## 10.3 Shape Rules

```text
Static rink/boards can use mesh or boxes.
Dynamic puck should use sphere/cylinder approximation depending gameplay choice.
Players/goalies should use capsule.
Goal volumes should use trigger boxes.
```

---

# 11. Physics World

## 11.1 Files

```text
PhysicsWorld.hpp/cpp
JoltPhysicsWorld.hpp/cpp
```

## 11.2 API

Implement:

```cpp
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    Status Init(const PhysicsSettings& settings);
    void Shutdown();

    bool IsInitialized() const;

    void CreateBodiesFromScene(Scene& scene);
    void DestroyBodies();

    void Step(float fixedDeltaSeconds);

    void SyncSceneToPhysics(Scene& scene);
    void SyncPhysicsToScene(Scene& scene);

    PhysicsBodyHandle CreateBody(Entity entity);
    void DestroyBody(Entity entity);
    bool HasBody(Entity entity) const;

    void SetBodyTransform(Entity entity, const glm::vec3& position, const glm::quat& rotation);
    void SetLinearVelocity(Entity entity, const glm::vec3& velocity);
    void AddForce(Entity entity, const glm::vec3& force);
    void AddImpulse(Entity entity, const glm::vec3& impulse);

    glm::vec3 GetLinearVelocity(Entity entity) const;
    glm::vec3 GetAngularVelocity(Entity entity) const;

    std::vector<PhysicsContactEvent> DrainContactEvents();
    std::vector<PhysicsTriggerEvent> DrainTriggerEvents();

    bool Raycast(const RaycastRequest& request, RaycastHit& outHit) const;
    std::vector<RaycastHit> RaycastAll(const RaycastRequest& request) const;

    bool OverlapSphere(const OverlapSphereRequest& request, std::vector<OverlapHit>& outHits) const;
    bool OverlapBox(const OverlapBoxRequest& request, std::vector<OverlapHit>& outHits) const;

    void CollectDebugDraw(PhysicsDebugDrawList& outDrawList) const;
};
```

## 11.3 Scene Sync Rules

`SyncSceneToPhysics`:

```text
creates missing bodies
destroys bodies for removed entities
updates kinematic body targets from TransformComponent
updates changed static bodies when editor changes transform
```

`SyncPhysicsToScene`:

```text
writes dynamic body transforms back to TransformComponent
does not overwrite static bodies unnecessarily
does not overwrite editor transforms when simulation disabled
```

## 11.4 Body Creation Rules

An entity creates a physics body if it has:

```text
RigidBodyComponent
and at least one collider component
```

Collider components:

```text
BoxColliderComponent
SphereColliderComponent
CapsuleColliderComponent
CylinderColliderComponent
MeshColliderComponent
```

If no collider exists, validation should warn/error.

---

# 12. Physics Scene/System Integration

## 12.1 PhysicsScene

Files:

```text
PhysicsScene.hpp/cpp
```

Purpose:

```text
Owns a PhysicsWorld for a Scene.
Controls lifecycle and stepping.
```

API:

```cpp
class PhysicsScene {
public:
    Status Init(Scene& scene, const PhysicsSettings& settings);
    void Shutdown();

    void OnSceneLoaded(Scene& scene);
    void OnSceneUnloaded();

    void OnSimulationStart(Scene& scene);
    void OnSimulationStop(Scene& scene);

    void OnFixedUpdate(Scene& scene, float fixedDeltaSeconds);

    PhysicsWorld& World();
};
```

## 12.2 PhysicsSystem

Files:

```text
PhysicsSystem.hpp/cpp
```

Integrates with Phase 2 Scene system:

```cpp
class PhysicsSystem final : public System {
public:
    explicit PhysicsSystem(PhysicsSettings settings);

    void OnStart(Scene& scene) override;
    void OnStop(Scene& scene) override;
    void OnFixedUpdate(Scene& scene, float fixedDeltaTime) override;

    PhysicsWorld& World();

private:
    PhysicsScene m_PhysicsScene;
};
```

Apps can add `PhysicsSystem` to scene when physics should run.

## 12.3 Mode Behavior

SceneMode behavior:

```text
Edit: physics bodies may exist for preview/debug, but not automatically simulated unless editor preview is enabled
Play: physics simulates on fixed update
Server: physics simulates on fixed update headlessly
ClientPrediction: physics can simulate prediction-ready local scene later
```

---

# 13. Collision and Trigger Events

## 13.1 Files

```text
PhysicsEvents.hpp/cpp
JoltContactListener.hpp/cpp
```

## 13.2 Contact Events

Implement:

```cpp
struct PhysicsContactEvent {
    UUID entityA;
    UUID entityB;

    glm::vec3 contactPoint{0.0f};
    glm::vec3 contactNormal{0.0f};

    float impulse = 0.0f;

    enum class Type {
        Started,
        Persisted,
        Ended
    } type = Type::Started;
};
```

## 13.3 Trigger Events

Implement:

```cpp
struct PhysicsTriggerEvent {
    UUID triggerEntity;
    UUID otherEntity;

    enum class Type {
        Entered,
        Exited
    } type = Type::Entered;
};
```

## 13.4 Requirements

```text
contact start event
contact persist event if feasible
contact end event
trigger enter event
trigger exit event
events map Jolt BodyID back to Entity UUID
events are drained each tick
events are safe if entity is destroyed
```

Do not implement hockey scoring or gameplay reactions in this phase.

---

# 14. Physics Queries

## 14.1 Files

```text
PhysicsQueries.hpp/cpp
```

## 14.2 Raycast

Implement:

```cpp
struct RaycastRequest {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    float maxDistance = 1000.0f;
    uint32_t layerMask = 0xFFFFFFFF;
    bool includeTriggers = false;
};

struct RaycastHit {
    UUID entity;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    float distance = 0.0f;
};
```

## 14.3 Overlap Queries

Implement:

```cpp
struct OverlapSphereRequest {
    glm::vec3 center{0.0f};
    float radius = 1.0f;
    uint32_t layerMask = 0xFFFFFFFF;
    bool includeTriggers = true;
};

struct OverlapBoxRequest {
    glm::vec3 center{0.0f};
    glm::vec3 halfExtents{1.0f};
    glm::quat rotation{};
    uint32_t layerMask = 0xFFFFFFFF;
    bool includeTriggers = true;
};

struct OverlapHit {
    UUID entity;
};
```

## 14.4 Shape Casts

If feasible, implement:

```text
sphere cast
capsule cast
box cast
```

If not feasible in this phase, leave a clean API seam but do not claim shape casts are complete.

---

# 15. Physics Debug Draw

## 15.1 Files

```text
PhysicsDebugDraw.hpp/cpp
JoltDebugRenderer.hpp/cpp
```

## 15.2 Debug Draw Data

Implement renderer-independent debug data:

```cpp
struct PhysicsDebugLine {
    glm::vec3 a;
    glm::vec3 b;
    glm::vec4 color;
};

struct PhysicsDebugDrawList {
    std::vector<PhysicsDebugLine> lines;

    void Clear();
};
```

## 15.3 Requirements

```text
draw collider wireframes
draw rigid body centers
draw trigger volumes with different color
draw contact points/normals if enabled
draw broadphase bounds if feasible
```

The physics library should only produce debug draw data.

Editor/game client bridge sends that data to renderer `DebugDraw`.

Server does not draw debug visuals.

---

# 16. Physics Validation

## 16.1 Files

```text
PhysicsValidation.hpp/cpp
```

## 16.2 Required Checks

Validate:

```text
RigidBodyComponent without collider
collider without RigidBodyComponent
negative mass
zero/negative collider sizes
invalid material name
invalid physics layer
dynamic mesh collider marked non-convex
trigger collider without TriggerComponent warning
Goal trigger setup warning if missing trigger
Puck physics setup warning if missing rigid body/collider/material
Player spawn preview warning if player capsule defaults invalid
```

Integrate with editor Scene Validation Panel.

---

# 17. Hockey-Specific Physics Setup

This phase does not implement gameplay, but it must provide the physics setup needed by gameplay later.

## 17.1 Puck Setup

Recommended entity components:

```text
PuckComponent
RigidBodyComponent:
  type Dynamic
  mass tuned placeholder value
  useGravity true or false depending rink design
  layer Puck
  materialName PuckRubber

SphereColliderComponent or CylinderColliderComponent:
  radius/height matching puck scale
```

Physics support required:

```text
puck can move
puck collides with boards/rink/players/goal triggers
puck bounces off boards
puck can enter trigger volume
```

No scoring rule in this phase.

## 17.2 Player Setup

Recommended player physics representation:

```text
RigidBodyComponent:
  type Kinematic or Dynamic depending design
  layer Player
  materialName PlayerBody

CapsuleColliderComponent:
  player-sized capsule
```

Physics support required:

```text
player capsule collides with rink/boards/puck
player can be moved kinematically by future gameplay
```

No movement gameplay in this phase.

## 17.3 Goalie Setup

Recommended:

```text
RigidBodyComponent:
  type Kinematic or Dynamic
  layer Goalie
  materialName GoalieBody

CapsuleColliderComponent:
  goalie-sized capsule
```

## 17.4 Rink/Boards Setup

Recommended:

```text
RigidBodyComponent:
  type Static
  layer Rink
  materialName Boards or Ice

BoxColliderComponent/MeshColliderComponent:
  boards
  walls
  ice plane
```

## 17.5 Goal Trigger Setup

Recommended:

```text
GoalComponent
RigidBodyComponent:
  type Static
  layer Trigger
  materialName Trigger

BoxColliderComponent:
  isTrigger true

TriggerComponent:
  tag = "Goal"
  detectPuck = true
```

Physics should generate trigger enter/exit events.

Gameplay later decides whether this means a goal.

---

# 18. Editor Integration

## 18.1 Inspector

Update inspector/metadata to edit:

```text
RigidBodyComponent
BoxColliderComponent
SphereColliderComponent
CapsuleColliderComponent
CylinderColliderComponent
MeshColliderComponent
TriggerComponent
CharacterControllerComponent if implemented
```

Fields must support:

```text
RigidBodyType enum
PhysicsLayer enum
materialName string/dropdown if material registry exists
mass
damping
gravity toggle
sleeping toggle
collider sizes
trigger toggles
```

## 18.2 Scene Viewport

Editor must visualize:

```text
box colliders
sphere colliders
capsule colliders
cylinder colliders
mesh collider bounds/wireframe if feasible
trigger volumes
rigid body centers
contact points during preview if enabled
```

Use `PhysicsDebugDrawList` bridged to renderer debug draw.

## 18.3 Editor Physics Preview

Add controls:

```text
Preview Physics toggle
Step Physics
Reset Physics Preview
Show Colliders toggle
Show Triggers toggle
Show Contacts toggle
```

Preview rules:

```text
Edit mode does not simulate by default.
When preview enabled, create physics world from scene.
Step physics using fixed delta.
Allow reset to original scene transforms.
Do not permanently modify scene unless user applies changes.
```

## 18.4 Hockey Placement Tools Update

Update hockey tools to optionally add physics components:

```text
Puck tool adds puck rigid body/collider defaults.
Goal tool adds goal trigger collider defaults.
Rink tool adds board/ice colliders.
Spawn tools may show capsule preview but do not add player runtime bodies unless requested.
```

---

# 19. App Integration

## 19.1 Game Client

Update `HockeyGameClient`.

Required behavior:

```text
load physics settings
initialize Physics
add PhysicsSystem to Play scene if physics enabled
run physics fixed update through scene lifecycle
sync physics debug draw if debug enabled
shutdown physics
```

No hockey gameplay rules.

## 19.2 Map Editor

Update `HockeyMapEditor`.

Required behavior:

```text
load physics settings
initialize Physics
support physics preview
draw colliders/triggers/debug physics
edit physics components
validate physics scene
shutdown physics
```

## 19.3 Dedicated Server

Update `HockeyDedicatedServer`.

Required behavior:

```text
load physics settings
initialize Physics
add PhysicsSystem to Server scene
run physics every fixed tick
drain/log physics event counts if debug enabled
shutdown physics
remain headless
no renderer/editor dependency
```

---

# 20. Tests

Create:

```text
HockeyPhysicsTests
```

Use simple test harness.

## 20.1 PhysicsInitTests

Test:

```text
Physics::Init works
Physics::Shutdown works
double init safe or controlled
world init works
world shutdown works
```

## 20.2 PhysicsComponentTests

Test:

```text
RigidBodyComponent defaults valid
BoxColliderComponent defaults valid
SphereColliderComponent defaults valid
CapsuleColliderComponent defaults valid
TriggerComponent defaults valid
components serialize/deserialize
metadata registered
```

## 20.3 PhysicsWorldTests

Test:

```text
create world
step world
set gravity
create bodies from scene
destroy bodies
body mapping entity <-> physics handle works
```

## 20.4 RigidBodyTests

Test:

```text
static body does not move from gravity
dynamic body moves under gravity
kinematic body follows set transform
linear velocity set/get
impulse changes velocity
locked axes restrict motion if implemented
```

## 20.5 ColliderTests

Test:

```text
box collider creates shape
sphere collider creates shape
capsule collider creates shape
cylinder collider creates shape
invalid collider size validation fails
mesh collider path handles missing asset gracefully
```

## 20.6 PhysicsMaterialTests

Test:

```text
built-in materials exist
ice material low friction
puck material restitution > default
invalid material detected
material assigned to body
```

## 20.7 CollisionLayerTests

Test:

```text
default collision matrix expected pairs
disabled layer pair does not collide
trigger layer works
puck collides with rink
puck collides with goal trigger
```

## 20.8 QueryTests

Test:

```text
raycast hits box
raycast misses when direction wrong
raycast respects layer mask
overlap sphere detects body
overlap box detects body
trigger inclusion flag works
```

## 20.9 TriggerTests

Test:

```text
trigger enter event generated
trigger exit event generated
trigger detects puck
trigger ignores disabled layer
```

## 20.10 ContactEventTests

Test:

```text
contact started event generated
contact ended event generated
contact includes entity UUIDs
contact point/normal valid enough
destroyed entity does not crash event drain
```

## 20.11 SceneSyncTests

Test:

```text
scene transform creates physics body transform
dynamic physics body writes back to scene transform
kinematic scene transform writes to physics
destroyed entity removes body
added collider creates body on sync
```

## 20.12 HockeyPhysicsSetupTests

Test:

```text
puck setup valid
player capsule setup valid
goal trigger setup valid
rink collider setup valid
puck collides with boards
puck enters goal trigger
```

## 20.13 HeadlessServerPhysicsTests

Test:

```text
server scene creates physics world
server fixed tick steps physics
server does not require renderer
server physics shuts down cleanly
```

---

# 21. Implementation Order for Cursor

Implement in this exact order.

## Step 1 — Dependency and Target Setup

```text
1. Add Jolt dependency to vcpkg.json.
2. Create libs/physics folder.
3. Create libs/physics/CMakeLists.txt.
4. Add hockey_physics target.
5. Create apps/physics_tests target.
6. Update root CMakeLists.txt.
7. Link client/editor/server/tests to hockey_physics.
8. Confirm server still does not link renderer/editor/ImGui.
9. Configure/build before complex code.
```

## Step 2 — Settings and Init

```text
1. Add PhysicsSettings.
2. Add config load/save.
3. Add Physics init/shutdown wrapper.
4. Add JoltCommon.
5. Add PhysicsInitTests.
```

## Step 3 — Layers and Materials

```text
1. Add PhysicsLayer.
2. Add collision matrix.
3. Add Jolt broadphase/object layer mapping.
4. Add PhysicsMaterial.
5. Add built-in materials.
6. Add CollisionLayerTests.
7. Add PhysicsMaterialTests.
```

## Step 4 — Components

```text
1. Add PhysicsComponents.
2. Add RigidBodyComponent.
3. Add collider components.
4. Add TriggerComponent.
5. Add CharacterControllerComponent if feasible.
6. Add serialization.
7. Add metadata registration.
8. Add PhysicsComponentTests.
```

## Step 5 — Shapes and Bodies

```text
1. Add PhysicsBody.
2. Add PhysicsShape.
3. Add JoltShapeFactory.
4. Implement box/sphere/capsule/cylinder shapes.
5. Implement mesh collider path gracefully.
6. Implement body creation/destruction.
7. Add RigidBodyTests.
8. Add ColliderTests.
```

## Step 6 — PhysicsWorld

```text
1. Add PhysicsWorld.
2. Add JoltPhysicsWorld.
3. Create Jolt world.
4. Implement Step.
5. Implement CreateBodiesFromScene.
6. Implement DestroyBodies.
7. Implement body mapping.
8. Implement SyncSceneToPhysics.
9. Implement SyncPhysicsToScene.
10. Add PhysicsWorldTests.
11. Add SceneSyncTests.
```

## Step 7 — Events

```text
1. Add PhysicsEvents.
2. Add JoltContactListener.
3. Implement contact events.
4. Implement trigger events.
5. Implement event drain.
6. Add ContactEventTests.
7. Add TriggerTests.
```

## Step 8 — Queries

```text
1. Add PhysicsQueries.
2. Implement Raycast.
3. Implement RaycastAll.
4. Implement OverlapSphere.
5. Implement OverlapBox.
6. Implement shape casts if feasible.
7. Add QueryTests.
```

## Step 9 — Debug Draw

```text
1. Add PhysicsDebugDraw.
2. Add JoltDebugRenderer.
3. Collect collider wireframes.
4. Collect trigger wireframes.
5. Collect contact normals if enabled.
6. Bridge editor/client to renderer debug draw.
```

## Step 10 — PhysicsSystem and Scene Integration

```text
1. Add PhysicsScene.
2. Add PhysicsSystem.
3. Add Scene lifecycle integration.
4. Add mode-specific behavior.
5. Add client integration.
6. Add editor integration.
7. Add server integration.
8. Add HeadlessServerPhysicsTests.
```

## Step 11 — Editor Integration

```text
1. Add physics component inspectors.
2. Add collider visualization.
3. Add physics preview controls.
4. Add scene validation integration.
5. Update hockey placement tools to add physics defaults.
6. Add HockeyPhysicsSetupTests.
```

## Step 12 — Final Verification

```text
1. Build Debug Windows.
2. Build Release Windows.
3. Build Debug Linux.
4. Build Release Linux.
5. Run HockeyCoreTests.
6. Run HockeyECSTests.
7. Run HockeyRendererTests.
8. Run HockeyEditorTests.
9. Run HockeyAssetTests.
10. Run HockeyPhysicsTests.
11. Run client.
12. Run editor.
13. Run server.
14. Verify server headless physics.
15. Verify no networking/gameplay code was added.
```

---

# 22. Verification Commands

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
./scripts/linux/run_client.sh
./scripts/linux/run_editor.sh
./scripts/linux/run_server.sh
```

---

# 23. Phase 6 Definition of Done

Phase 6 is complete only when all of this is true:

```text
Jolt dependency added
hockey_physics builds
HockeyPhysicsTests builds
HockeyPhysicsTests pass
Physics init/shutdown works
PhysicsSettings load from config
physics world creates/shuts down
collision layers work
collision filtering works
physics materials work
RigidBodyComponent serializes
collider components serialize
TriggerComponent serializes
physics metadata registered
body creation from scene works
body destruction works
scene-to-physics sync works
physics-to-scene sync works
static bodies work
dynamic bodies work
kinematic bodies work
box colliders work
sphere colliders work
capsule colliders work
cylinder colliders work
mesh collider path handles assets/missing assets cleanly
raycasts work
overlap sphere works
overlap box works
contact events work
trigger events work
event drain works
physics debug draw data is generated
editor visualizes colliders/triggers
editor physics preview works
physics validation works
hockey puck setup is valid
hockey player capsule setup is valid
hockey goalie setup is valid
hockey rink/board setup is valid
hockey goal trigger setup is valid
client can run physics
editor can preview physics
server can run physics headlessly
server does not link renderer/editor/ImGui
Windows build works
Linux build works
no networking code exists
no hockey gameplay simulation exists
no final animation/audio/UI exists
```

---

# 24. Cursor Completion Instruction

When Phase 6 is complete, Cursor should report:

```text
Phase 6 complete.

Implemented:
- hockey_physics
- HockeyPhysicsTests
- Jolt initialization/shutdown
- physics settings/config
- physics world
- physics scene/system integration
- collision layers/filtering
- physics materials
- rigid body component
- collider components
- trigger component
- Jolt body/shape creation
- scene-to-physics sync
- physics-to-scene sync
- contact events
- trigger events
- raycasts
- overlap queries
- physics debug draw
- physics validation
- editor collider visualization
- editor physics preview
- hockey-specific physics setup defaults
- client/editor/server physics integration

Verified:
- physics tests
- client physics path
- editor physics preview
- server headless physics
- Windows/Linux builds
- no future-phase networking/gameplay systems were implemented
```
