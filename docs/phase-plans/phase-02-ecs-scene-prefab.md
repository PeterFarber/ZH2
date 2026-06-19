# Phase 2 Plan — Complete ECS, GameObject Model, Scene System, Prefabs, and Component Metadata

File path recommendation:

```text
docs/phase-plans/phase-02-ecs-scene-prefab.md
```

AI instruction:

```text
Read `docs/phase-plans/phase-02-ecs-scene-prefab.md` and implement Phase 2 exactly. Do not implement Vulkan, renderer, physics, networking, GameNetworkingSockets, hockey gameplay simulation, ImGui editor UI, audio, animation, or the final asset pipeline during this phase. Work step by step, update CMake whenever files are added, add tests, and keep Windows/Linux compatibility.
```

---

# 0. Phase Summary

Phase 2 builds the complete ECS, GameObject, scene, prefab, hierarchy, transform, serialization, validation, metadata, and scene-lifecycle foundation.

This phase builds the data model that will later be used by:

```text
game client
map editor
dedicated server
renderer
physics
networking
gameplay
asset pipeline
```

Phase 2 must produce:

```text
hockey_ecs
HockeyECSTests
```

Phase 2 updates these apps:

```text
HockeyGameClient
HockeyMapEditor
HockeyDedicatedServer
```

Phase 2 must be complete enough that the map editor can later create Unity-style GameObjects, add/remove/edit components, save/load scenes, and manage prefabs.

Phase 2 must not contain:

```text
Vulkan
renderer implementation
physics simulation
Jolt
networking
GameNetworkingSockets
hockey gameplay simulation
ImGui editor UI
audio
animation
final asset pipeline
```

---

# 1. Architecture Rules

## 1.1 Dependency Direction

Phase 2 adds:

```text
hockey_ecs -> hockey_core
```

Allowed:

```text
apps/game_client       -> hockey_core, hockey_ecs
apps/map_editor        -> hockey_core, hockey_ecs
apps/dedicated_server  -> hockey_core, hockey_ecs
apps/ecs_tests         -> hockey_core, hockey_ecs
```

Forbidden:

```text
hockey_ecs -> hockey_renderer
hockey_ecs -> hockey_physics
hockey_ecs -> hockey_networking
hockey_ecs -> hockey_gameplay
hockey_ecs -> hockey_editor
hockey_ecs -> Vulkan
hockey_ecs -> SDL windowing
hockey_ecs -> ImGui
hockey_ecs -> Jolt
hockey_ecs -> GameNetworkingSockets
```

`hockey_ecs` may include `glm` because transforms and vectors are fundamental component data.

## 1.2 System Ownership

`hockey_ecs` owns:

```text
entities
components as data
scenes
hierarchy
transforms
component metadata
component serialization
scene serialization
scene validation
prefabs
prefab overrides
scene events
system/lifecycle interfaces
```

`hockey_ecs` does not own:

```text
rendering behavior
physics simulation behavior
network packet behavior
hockey match rules
editor panel UI
asset importing/cooking
```

---

# 2. Dependencies

Update `vcpkg.json`.

Add:

```json
"entt",
"yaml-cpp"
```

Final Phase 2 dependency list should include Phase 1 dependencies plus:

```text
entt
yaml-cpp
```

Dependency purpose:

```text
EnTT      ECS storage/backend
yaml-cpp  scene/prefab serialization
```

Do not add renderer, physics, networking, ImGui, or asset-pipeline dependencies.

---

# 3. Target Project Structure

Add:

```text
libs/
  ecs/
    CMakeLists.txt
    include/Hockey/ECS/
      Entity.hpp
      Scene.hpp
      SceneMode.hpp
      Components.hpp

      Transform.hpp
      Hierarchy.hpp

      ComponentMetadata.hpp
      ComponentRegistry.hpp
      ComponentSerializer.hpp

      SceneSerializer.hpp
      SceneValidator.hpp
      SceneEvents.hpp

      Prefab.hpp
      PrefabSerializer.hpp
      PrefabOverride.hpp

      System.hpp

      YAML.hpp

    src/
      Entity.cpp
      Scene.cpp

      Transform.cpp
      Hierarchy.cpp

      ComponentMetadata.cpp
      ComponentRegistry.cpp
      ComponentSerializer.cpp

      SceneSerializer.cpp
      SceneValidator.cpp
      SceneEvents.cpp

      Prefab.cpp
      PrefabSerializer.cpp
      PrefabOverride.cpp

      System.cpp

      YAML.cpp

apps/
  ecs_tests/
    CMakeLists.txt
    src/
      ECSTestsMain.cpp
      Test.hpp
      EntityTests.cpp
      ComponentTests.cpp
      HierarchyTests.cpp
      TransformTests.cpp
      SceneSerializationTests.cpp
      SceneValidationTests.cpp
      PrefabTests.cpp
      ComponentMetadataTests.cpp
      SceneLifecycleTests.cpp
      SceneEventTests.cpp

data/
  raw/
    scenes/
      main_rink.scene.yaml
    prefabs/
```

Update root `CMakeLists.txt`:

```cmake
add_subdirectory(libs/ecs)

if(HOCKEY_BUILD_TESTS)
    add_subdirectory(apps/core_tests)
    add_subdirectory(apps/ecs_tests)
endif()
```

If `apps/core_tests` is already inside the `if`, do not duplicate it.

---

# 4. CMake Requirements

## 4.1 `libs/ecs/CMakeLists.txt`

Create `hockey_ecs`.

Required:

```cmake
find_package(EnTT CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_library(hockey_ecs STATIC)

target_sources(hockey_ecs
    PRIVATE
        src/Entity.cpp
        src/Scene.cpp
        src/Transform.cpp
        src/Hierarchy.cpp
        src/ComponentMetadata.cpp
        src/ComponentRegistry.cpp
        src/ComponentSerializer.cpp
        src/SceneSerializer.cpp
        src/SceneValidator.cpp
        src/SceneEvents.cpp
        src/Prefab.cpp
        src/PrefabSerializer.cpp
        src/PrefabOverride.cpp
        src/System.cpp
        src/YAML.cpp
)

target_include_directories(hockey_ecs
    PUBLIC include
)

target_link_libraries(hockey_ecs
    PUBLIC
        hockey_core
        EnTT::EnTT
        yaml-cpp::yaml-cpp
)
```

Add platform-safe warnings matching Phase 1 conventions.

## 4.2 App Links

Update:

```text
HockeyGameClient       links hockey_core hockey_ecs
HockeyMapEditor        links hockey_core hockey_ecs
HockeyDedicatedServer  links hockey_core hockey_ecs
```

Do not link renderer, physics, networking, or editor UI.

## 4.3 `apps/ecs_tests/CMakeLists.txt`

Create `HockeyECSTests`.

It must link:

```text
hockey_core
hockey_ecs
```

Do not use an external test framework unless the project already has one. Use the same simple internal test style from Phase 1.

---

# 5. Core ECS Concepts

## 5.1 Runtime Entity Handle vs Stable UUID

Every entity has two identities:

```text
entt::entity
  Runtime-only handle.
  Fast ECS storage handle.
  Not stable across save/load.

UUID
  Stable project/scene ID.
  Saved in scenes and prefabs.
  Used by editor, serialization, prefab tracking, future networking mapping, and references.
```

Rules:

```text
Every entity must have IDComponent.
Every entity UUID must be nonzero.
Every entity UUID must be unique within a scene.
UUIDs must survive scene save/load.
Duplicating entities generates new UUIDs.
Prefab instancing generates new UUIDs mapped from prefab UUIDs.
```

## 5.2 Entity Wrapper

Use EnTT internally but expose an engine wrapper.

Normal code should use:

```cpp
Entity entity = scene.CreateEntity("Player");
entity.AddComponent<TeamComponent>();
```

Avoid exposing raw registry calls everywhere.

Allowed raw registry access:

```text
performance-critical system iteration
tests
controlled internal implementation
```

---

# 6. Required Components

All components live in `Components.hpp` unless a component group becomes large enough to split later.

## 6.1 Core Components

Implement:

```cpp
struct IDComponent {
    UUID id;
};

struct NameComponent {
    std::string name = "GameObject";
};

struct ActiveComponent {
    bool active = true;
    bool activeInHierarchy = true;
};

struct TransformComponent {
    glm::vec3 localPosition{0.0f};
    glm::vec3 localRotation{0.0f}; // Euler degrees for editor friendliness
    glm::vec3 localScale{1.0f};

    glm::mat4 LocalMatrix() const;
};

struct ParentComponent {
    UUID parentId{};
};

struct ChildrenComponent {
    std::vector<UUID> children;
};
```

Rules:

```text
Every normal GameObject gets IDComponent.
Every normal GameObject gets NameComponent.
Every normal GameObject gets ActiveComponent.
Every normal GameObject gets TransformComponent.
Every normal GameObject gets ChildrenComponent.
ParentComponent only exists when entity has a parent.
ChildrenComponent always exists on normal GameObjects.
```

## 6.2 Prefab Component

Implement:

```cpp
struct PrefabComponent {
    UUID prefabAssetId;
    UUID sourceEntityId;
    std::filesystem::path sourcePath;
};
```

Rules:

```text
PrefabComponent is added to instantiated prefab entities.
sourceEntityId refers to original prefab entity UUID before remap.
prefabAssetId identifies the prefab asset/file.
sourcePath stores path to prefab file.
```

## 6.3 Hockey Marker Components

These are scene/editor marker components only. They do not simulate hockey gameplay.

Implement:

```cpp
enum class Team {
    None,
    Home,
    Away
};

enum class PlayerRole {
    Skater,
    Goalie
};

struct TeamComponent {
    Team team = Team::None;
};

struct PlayerRoleComponent {
    PlayerRole role = PlayerRole::Skater;
};

struct PuckComponent {
    bool startsInPlay = true;
};

struct GoalComponent {
    Team defendingTeam = Team::None;
};

struct SpawnPointComponent {
    Team team = Team::None;
    PlayerRole role = PlayerRole::Skater;
    int index = 0;
};

struct FaceoffSpotComponent {
    int index = 0;
};

struct RinkComponent {
    std::string rinkName = "Default Rink";
};

struct PlayAreaComponent {
    glm::vec3 halfExtents{30.0f, 5.0f, 60.0f};
};

struct CameraRigMarkerComponent {
    std::string purpose = "GameplayCamera";
};
```

Do not add actual movement, puck physics, shooting, passing, checking, or scoring logic in Phase 2.

---

# 7. Entity API

## 7.1 Files

```text
Entity.hpp
Entity.cpp
```

## 7.2 API

Implement:

```cpp
class Scene;

class Entity {
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);

    bool IsValid() const;
    explicit operator bool() const;

    UUID GetUUID() const;

    const std::string& GetName() const;
    void SetName(const std::string& name);

    bool IsActive() const;
    bool IsActiveInHierarchy() const;
    void SetActive(bool active);

    entt::entity Handle() const;
    Scene* GetScene() const;

    template<typename T, typename... Args>
    T& AddComponent(Args&&... args);

    template<typename T, typename... Args>
    T& AddOrReplaceComponent(Args&&... args);

    template<typename T>
    T& GetComponent();

    template<typename T>
    const T& GetComponent() const;

    template<typename T>
    bool HasComponent() const;

    template<typename T>
    void RemoveComponent();

    template<typename... T>
    bool HasAll() const;

    template<typename... T>
    bool HasAny() const;

    bool operator==(const Entity& other) const;
    bool operator!=(const Entity& other) const;

private:
    entt::entity m_Handle = entt::null;
    Scene* m_Scene = nullptr;
};
```

## 7.3 Entity Rules

```text
AddComponent asserts/fails if component already exists.
AddOrReplaceComponent is allowed for editor workflows.
RemoveComponent cannot remove IDComponent.
RemoveComponent cannot remove TransformComponent from normal GameObjects.
RemoveComponent cannot remove ChildrenComponent from normal GameObjects.
GetComponent asserts/fails if missing component.
IsValid checks both scene pointer and registry validity.
```

Component add/remove should push scene events.

---

# 8. Scene Mode

## 8.1 File

```text
SceneMode.hpp
```

## 8.2 Enum

Implement:

```cpp
enum class SceneMode {
    Edit,
    Play,
    Server,
    ClientPrediction
};
```

Mode usage:

```text
Edit              map editor editing mode
Play              local/game-client runtime scene
Server            authoritative dedicated server scene
ClientPrediction  future client prediction/interpolation scene
```

---

# 9. Scene API

## 9.1 Files

```text
Scene.hpp
Scene.cpp
```

## 9.2 API

Implement:

```cpp
class Scene {
public:
    explicit Scene(std::string name = "Untitled Scene");
    ~Scene();

    const std::string& GetName() const;
    void SetName(std::string name);

    SceneMode GetMode() const;
    void SetMode(SceneMode mode);

    Entity CreateEntity(const std::string& name = "GameObject");
    Entity CreateEntityWithUUID(UUID id, const std::string& name);

    Entity DuplicateEntity(Entity source);

    void DestroyEntity(Entity entity);
    void DestroyEntityRecursive(Entity entity);

    Entity FindEntityByUUID(UUID id);
    Entity FindEntityByName(const std::string& name);

    bool IsValid(Entity entity) const;
    bool ContainsUUID(UUID id) const;

    void SetParent(Entity child, Entity parent, bool keepWorldTransform = true);
    void RemoveParent(Entity child, bool keepWorldTransform = true);
    bool IsDescendantOf(Entity child, Entity possibleParent) const;

    glm::mat4 GetLocalTransform(Entity entity) const;
    glm::mat4 GetWorldTransform(Entity entity) const;
    void SetWorldTransform(Entity entity, const glm::mat4& worldTransform);

    std::vector<Entity> GetRootEntities() const;
    std::vector<Entity> GetChildren(Entity entity) const;

    void OnRuntimeStart();
    void OnRuntimeStop();

    void OnSimulationStart();
    void OnSimulationStop();

    void OnUpdate(float deltaTime);
    void OnFixedUpdate(float fixedDeltaTime);

    void AddSystem(std::unique_ptr<System> system);
    void ClearSystems();

    const std::vector<SceneEvent>& GetPendingEvents() const;
    void ClearPendingEvents();

    entt::registry& Registry();
    const entt::registry& Registry() const;

private:
    std::string m_Name;
    SceneMode m_Mode = SceneMode::Edit;

    entt::registry m_Registry;
    std::unordered_map<UUID, entt::entity> m_EntityMap;

    std::vector<std::unique_ptr<System>> m_Systems;
    std::vector<SceneEvent> m_PendingEvents;
};
```

## 9.3 Create Entity Behavior

`CreateEntity("GameObject")` must add:

```text
IDComponent
NameComponent
ActiveComponent
TransformComponent
ChildrenComponent
```

Default values:

```text
Name: requested name or GameObject
Active: true
ActiveInHierarchy: true
Local position: 0,0,0
Local rotation: 0,0,0
Local scale: 1,1,1
Children: empty
```

## 9.4 Destroy Behavior

Implement two destroy paths:

```cpp
void DestroyEntity(Entity entity);
void DestroyEntityRecursive(Entity entity);
```

Rules:

```text
DestroyEntity:
  destroys the entity only
  detaches children to root
  preserves children world transform
  removes destroyed UUID from entity map
  removes destroyed entity from parent child list
  pushes EntityDestroyed event

DestroyEntityRecursive:
  destroys entity and all descendants
  removes all UUIDs from entity map
  pushes EntityDestroyed events
```

## 9.5 Duplicate Behavior

`DuplicateEntity` must:

```text
copy selected entity
copy all children recursively
generate new UUIDs
copy all Phase 2 components
preserve local transforms
preserve hierarchy
not preserve PrefabComponent unless explicitly duplicating prefab instance as prefab instance
push EntityCreated events for new entities
```

Acceptable implementation strategy:

```text
serialize subtree to memory
remap UUIDs
deserialize into scene
```

or explicit component copy.

---

# 10. Transform System

## 10.1 Files

```text
Transform.hpp
Transform.cpp
```

## 10.2 API

Implement:

```cpp
glm::mat4 ComposeTransform(
    const glm::vec3& position,
    const glm::vec3& rotationDegrees,
    const glm::vec3& scale
);

void DecomposeTransform(
    const glm::mat4& matrix,
    glm::vec3& position,
    glm::vec3& rotationDegrees,
    glm::vec3& scale
);
```

## 10.3 Behavior

Use local transforms as stored data.

World transform:

```text
No parent:
  world = local

With parent:
  world = parentWorld * local
```

Reparent with keep world transform:

```text
newLocal = inverse(newParentWorld) * oldWorld
```

Unparent with keep world transform:

```text
newLocal = oldWorld
```

## 10.4 Done Criteria

```text
Parent translation moves child in world space.
Child local transform remains relative.
Reparent with keepWorldTransform preserves world transform.
Unparent with keepWorldTransform preserves world transform.
```

---

# 11. Hierarchy System

## 11.1 Files

```text
Hierarchy.hpp
Hierarchy.cpp
```

Hierarchy functions may be implemented as Scene methods and helpers.

## 11.2 Required Behavior

Implement:

```text
SetParent
RemoveParent
GetChildren
GetRootEntities
IsDescendantOf
```

Rules:

```text
entity cannot parent itself
entity cannot be parented to its descendant
parent must be valid
child must be valid
setting same parent does nothing
parent and child lists must stay synchronized
ParentComponent uses UUID
ChildrenComponent stores UUID list
```

## 11.3 Root Entities

Root entity:

```text
entity with no ParentComponent
```

`GetRootEntities()` returns all valid entities without ParentComponent.

---

# 12. Active State Propagation

`ActiveComponent` includes:

```text
active
activeInHierarchy
```

Rules:

```text
active = local user-controlled value
activeInHierarchy = active && parent.activeInHierarchy
```

When an entity active state changes:

```text
recalculate activeInHierarchy for entity and all descendants
push relevant scene events if needed
```

Systems should later be able to skip inactive-in-hierarchy entities.

---

# 13. Scene Events

## 13.1 Files

```text
SceneEvents.hpp
SceneEvents.cpp
```

## 13.2 Types

Implement:

```cpp
enum class SceneEventType {
    EntityCreated,
    EntityDestroyed,
    ComponentAdded,
    ComponentRemoved,
    ParentChanged,
    EntityRenamed,
    ActiveChanged
};

struct SceneEvent {
    SceneEventType type = SceneEventType::EntityCreated;
    UUID entityId{};
    std::string componentName;
};
```

Scene must expose:

```cpp
const std::vector<SceneEvent>& GetPendingEvents() const;
void ClearPendingEvents();
```

Events help editor/UI later without forcing editor code into ECS.

---

# 14. System Interface

## 14.1 Files

```text
System.hpp
System.cpp
```

## 14.2 API

Implement:

```cpp
class System {
public:
    virtual ~System() = default;

    virtual void OnStart(Scene& scene) {}
    virtual void OnStop(Scene& scene) {}
    virtual void OnUpdate(Scene& scene, float deltaTime) {}
    virtual void OnFixedUpdate(Scene& scene, float fixedDeltaTime) {}
};
```

Scene supports:

```cpp
void AddSystem(std::unique_ptr<System> system);
void ClearSystems();
```

Phase 2 does not add renderer/physics/gameplay systems.

The system interface exists so future phases can plug systems into scene lifecycle.

---

# 15. YAML Helpers

## 15.1 Files

```text
YAML.hpp
YAML.cpp
```

## 15.2 Required Helpers

Implement:

```cpp
YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& value);
YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& value);
YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& value);

bool ReadVec2(const YAML::Node& node, glm::vec2& out);
bool ReadVec3(const YAML::Node& node, glm::vec3& out);
bool ReadVec4(const YAML::Node& node, glm::vec4& out);

std::string TeamToString(Team team);
Team TeamFromString(const std::string& value);

std::string PlayerRoleToString(PlayerRole role);
PlayerRole PlayerRoleFromString(const std::string& value);
```

Use readable YAML.

Do not use binary scene formats in Phase 2.

---

# 16. Component Metadata

The metadata system is required for the future Unity-style Inspector.

## 16.1 Files

```text
ComponentMetadata.hpp
ComponentMetadata.cpp
ComponentRegistry.hpp
ComponentRegistry.cpp
```

## 16.2 Field Types

Implement:

```cpp
enum class FieldType {
    Bool,
    Int,
    Float,
    String,
    Vec2,
    Vec3,
    Vec4,
    Enum,
    UUID,
    Path
};
```

## 16.3 Field Metadata

Implement:

```cpp
struct FieldMetadata {
    std::string name;
    std::string displayName;
    FieldType type;

    size_t offset = 0;

    float minFloat = 0.0f;
    float maxFloat = 0.0f;
    float speed = 0.1f;

    int minInt = 0;
    int maxInt = 0;

    std::vector<std::string> enumNames;
    std::vector<int> enumValues;

    bool readOnly = false;
};
```

## 16.4 Component Metadata

Implement:

```cpp
struct ComponentMetadata {
    std::string name;
    std::string displayName;

    std::vector<FieldMetadata> fields;

    std::function<bool(Entity)> has;
    std::function<void(Entity)> add;
    std::function<void(Entity)> remove;
};
```

## 16.5 Component Registry

Implement:

```cpp
class ComponentRegistry {
public:
    static ComponentRegistry& Get();

    template<typename T>
    void RegisterComponent(ComponentMetadata metadata);

    const ComponentMetadata* FindByName(const std::string& name) const;
    const std::vector<ComponentMetadata>& All() const;

    void RegisterPhase2Components();

private:
    std::vector<ComponentMetadata> m_Components;
};
```

## 16.6 Required Registrations

Register metadata for:

```text
NameComponent
ActiveComponent
TransformComponent
TeamComponent
PlayerRoleComponent
PuckComponent
GoalComponent
SpawnPointComponent
FaceoffSpotComponent
RinkComponent
PlayAreaComponent
CameraRigMarkerComponent
PrefabComponent
```

`IDComponent` should be visible as read-only metadata or intentionally not removable/editable.

`ChildrenComponent` and `ParentComponent` can be visible as read-only or hidden from normal Add Component menu.

## 16.7 Rules

Metadata must allow future editor to:

```text
list components
show add component menu
detect whether entity has component
add component
remove component
iterate editable fields
know field type
know enum names/values
know read-only fields
```

---

# 17. Component Serialization

## 17.1 Files

```text
ComponentSerializer.hpp
ComponentSerializer.cpp
```

## 17.2 Requirements

Implement manual serializers.

Do not rely on automatic metadata-based serialization in Phase 2.

Manual serialization is explicit and safer.

## 17.3 Required Component Serialization

Serialize/deserialize:

```text
IDComponent
NameComponent
ActiveComponent
TransformComponent
ParentComponent
ChildrenComponent
PrefabComponent

TeamComponent
PlayerRoleComponent
PuckComponent
GoalComponent
SpawnPointComponent
FaceoffSpotComponent
RinkComponent
PlayAreaComponent
CameraRigMarkerComponent
```

## 17.4 API

Implement:

```cpp
class ComponentSerializer {
public:
    static void SerializeEntity(YAML::Emitter& out, Entity entity);
    static Entity DeserializeEntity(Scene& scene, const YAML::Node& node);

    static bool DeserializeCoreComponents(Entity entity, const YAML::Node& node);
    static bool DeserializeHockeyMarkerComponents(Entity entity, const YAML::Node& node);
};
```

If this API needs internal helper methods, keep them private/static in cpp.

---

# 18. Scene Serialization

## 18.1 Files

```text
SceneSerializer.hpp
SceneSerializer.cpp
```

## 18.2 File Extension

Use:

```text
.scene.yaml
```

## 18.3 API

```cpp
class SceneSerializer {
public:
    explicit SceneSerializer(Scene& scene);

    Status Serialize(const std::filesystem::path& path);
    Status Deserialize(const std::filesystem::path& path);

private:
    Scene& m_Scene;
};
```

## 18.4 Scene YAML Format

Required shape:

```yaml
Scene:
  Name: Main Rink
  Version: 1

Entities:
  - Entity: 173934557188382
    NameComponent:
      Name: Rink
    ActiveComponent:
      Active: true
    TransformComponent:
      Position: [0.0, 0.0, 0.0]
      Rotation: [0.0, 0.0, 0.0]
      Scale: [1.0, 1.0, 1.0]
    ChildrenComponent:
      Children:
        - 37291473291023
    RinkComponent:
      RinkName: "Default Rink"

  - Entity: 37291473291023
    NameComponent:
      Name: Puck Spawn
    ActiveComponent:
      Active: true
    TransformComponent:
      Position: [0.0, 0.05, 0.0]
      Rotation: [0.0, 0.0, 0.0]
      Scale: [1.0, 1.0, 1.0]
    ParentComponent:
      Parent: 173934557188382
    ChildrenComponent:
      Children: []
    PuckComponent:
      StartsInPlay: true
```

## 18.5 Serialization Rules

```text
write scene name
write scene version
write all entities
write stable UUIDs
write all core components
write all hockey marker components
write prefab components
write hierarchy references by UUID
write readable YAML
create parent directories
return Status
log errors
```

## 18.6 Deserialization Rules

```text
clear current scene before load
read scene name/version
create all entities with saved UUIDs
load core components
load hockey marker components
load prefab components
restore hierarchy after all entities exist
validate parent/child consistency
recalculate activeInHierarchy
return Status
log errors
```

---

# 19. Scene Validation

## 19.1 Files

```text
SceneValidator.hpp
SceneValidator.cpp
```

## 19.2 Types

Implement:

```cpp
struct SceneValidationIssue {
    enum class Severity {
        Warning,
        Error
    };

    Severity severity = Severity::Warning;
    std::string message;
    UUID entityId{};
};

class SceneValidator {
public:
    static std::vector<SceneValidationIssue> Validate(const Scene& scene);
    static bool HasErrors(const std::vector<SceneValidationIssue>& issues);
};
```

## 19.3 Required Validation Checks

Implement checks for:

```text
all entities have IDComponent
all entities have NameComponent
all normal entities have TransformComponent
all normal entities have ChildrenComponent
no duplicate UUIDs
ParentComponent parent UUID exists
ChildrenComponent child UUIDs exist
parent/child links match
no cyclic hierarchy
spawn point index >= 0
spawn point team/role valid
faceoff spot index >= 0
goal defending team is Home or Away
hockey scene warning if no puck marker exists
hockey scene warning if fewer than 2 goals exist
hockey scene warning if more than 2 goals exist
hockey scene warning if missing skater/goalie spawn markers
```

Warnings should not fail scene loading unless severe.

Errors should fail validation.

---

# 20. Prefab System

## 20.1 Files

```text
Prefab.hpp
Prefab.cpp
PrefabSerializer.hpp
PrefabSerializer.cpp
PrefabOverride.hpp
PrefabOverride.cpp
```

## 20.2 File Extension

Use:

```text
.prefab.yaml
```

## 20.3 Prefab API

```cpp
class Prefab {
public:
    Status SaveFromEntity(Scene& scene, Entity root, const std::filesystem::path& path);
    Result<Entity> Instantiate(Scene& targetScene, const std::filesystem::path& path);
};
```

## 20.4 Prefab YAML Format

Required shape:

```yaml
Prefab:
  Name: GoalNetPrefab
  Version: 1
  AssetId: 9938293477234

Root:
  Entity: 123456789

Entities:
  - Entity: 123456789
    NameComponent:
      Name: Goal Net
    TransformComponent:
      Position: [0.0, 0.0, 0.0]
      Rotation: [0.0, 0.0, 0.0]
      Scale: [1.0, 1.0, 1.0]
    ChildrenComponent:
      Children:
        - 987654321

  - Entity: 987654321
    NameComponent:
      Name: Goal Trigger
    ParentComponent:
      Parent: 123456789
    TransformComponent:
      Position: [0.0, 0.0, 0.0]
      Rotation: [0.0, 0.0, 0.0]
      Scale: [1.0, 1.0, 1.0]
    GoalComponent:
      DefendingTeam: Home
```

## 20.5 Instancing Rules

When instantiating prefab:

```text
load prefab file
generate new UUID for each prefab entity
build prefab UUID -> scene UUID remap
preserve hierarchy using remapped UUIDs
copy components
add PrefabComponent to each instance entity
return root entity
```

## 20.6 Prefab Overrides

Implement required override types:

```text
Transform field override
Name override
Active override
Hockey marker field override
```

Types:

```cpp
struct PrefabOverride {
    UUID entityId;
    std::string componentName;
    std::string fieldName;
    YAML::Node value;
};

class PrefabOverrideSet {
public:
    void AddOverride(const PrefabOverride& override);
    Status Apply(Scene& scene);
};
```

Override application should log and return failure for invalid entity/component/field.

---

# 21. Default Main Rink Scene

Create:

```text
data/raw/scenes/main_rink.scene.yaml
```

It must contain a valid marker-only hockey scene:

```text
Main Rink
  Home Goal
  Away Goal
  Puck Spawn
  Home Skater Spawn 0
  Home Skater Spawn 1
  Home Skater Spawn 2
  Home Goalie Spawn 0
  Away Skater Spawn 0
  Away Skater Spawn 1
  Away Skater Spawn 2
  Away Goalie Spawn 0
  Center Faceoff Spot
  Home Defensive Faceoff Spot
  Away Defensive Faceoff Spot
  Gameplay Camera Rig
```

This scene does not render and does not simulate gameplay yet.

It is data-only.

---

# 22. Config Updates

Update `client.toml`:

```toml
[scene]
startup_scene = "data/raw/scenes/main_rink.scene.yaml"
```

Update `editor.toml`:

```toml
[scene]
startup_scene = "data/raw/scenes/main_rink.scene.yaml"
autosave_enabled = true
autosave_interval_seconds = 120
```

Update `server.toml`:

```toml
[scene]
startup_scene = "data/raw/scenes/main_rink.scene.yaml"
validate_on_load = true
```

Phase 2 only loads these settings.

Do not implement editor UI autosave yet.

---

# 23. App Integration

## 23.1 Game Client

Update `HockeyGameClient`.

Required behavior:

```text
initialize core
create Scene in Play mode
load data/raw/scenes/main_rink.scene.yaml if configured and present
if missing, create empty Play-mode scene in memory
log scene name
log entity count
run scene OnUpdate each frame
do not save scenes by default
shutdown cleanly
```

Do not add rendering.

Do not add gameplay simulation.

## 23.2 Map Editor

Update `HockeyMapEditor`.

Required behavior:

```text
initialize core
create Scene in Edit mode
load startup scene if configured and present
if missing, create default editable scene in memory
support --scene <path> command-line override
allow temporary developer save command/hotkey if input exists
validate scene after load
validate scene after save
log hierarchy to console/log
shutdown cleanly
```

No ImGui UI in this phase.

## 23.3 Dedicated Server

Update `HockeyDedicatedServer`.

Required behavior:

```text
initialize core
create Scene in Server mode
load configured startup scene
validate scene
log validation warnings/errors
run scene OnFixedUpdate each server tick
stay headless
shutdown cleanly
```

Do not add networking.

Do not add gameplay simulation.

---

# 24. Tests

Create:

```text
HockeyECSTests
```

Use simple internal test harness.

## 24.1 Entity Tests

Test:

```text
CreateEntity adds ID/Name/Active/Transform/Children
CreateEntity UUID is unique
CreateEntityWithUUID preserves requested UUID
FindEntityByUUID works
FindEntityByName works
DestroyEntity removes entity
DestroyEntity removes UUID map entry
DestroyEntityRecursive destroys children
DuplicateEntity creates new UUIDs
DuplicateEntity copies components
```

## 24.2 Component Tests

Test:

```text
AddComponent works
Add duplicate component fails/asserts or returns controlled failure
AddOrReplaceComponent works
GetComponent works
HasComponent works
RemoveComponent works
Cannot remove IDComponent
Cannot remove TransformComponent from normal entity
Cannot remove ChildrenComponent from normal entity
Component add event fires
Component remove event fires
```

## 24.3 Hierarchy Tests

Test:

```text
SetParent works
RemoveParent works
Cannot parent entity to itself
Cannot parent entity to descendant
Parent child list updates
Child ParentComponent updates
DestroyEntity detaches children
DestroyEntityRecursive destroys children
GetRootEntities returns correct roots
GetChildren returns correct children
```

## 24.4 Transform Tests

Test:

```text
ComposeTransform works
DecomposeTransform works
LocalMatrix works
Parent transform affects child world transform
Reparent keepWorldTransform preserves world transform
Unparent keepWorldTransform preserves world transform
```

## 24.5 Active State Tests

Test:

```text
local active changes
activeInHierarchy updates
parent inactive makes child inactiveInHierarchy
reactivating parent updates descendants
```

## 24.6 Scene Serialization Tests

Test:

```text
Scene serializes to YAML
Scene deserializes from YAML
scene name preserved
UUIDs preserved
names preserved
active state preserved
transforms preserved
hierarchy preserved
hockey marker components preserved
prefab components preserved
```

## 24.7 Scene Validation Tests

Test:

```text
valid scene passes
duplicate UUID is error
broken parent is error
broken child is error
cycle is error
missing puck marker is warning
missing goals warning
invalid spawn index warning/error
```

## 24.8 Prefab Tests

Test:

```text
Save prefab from entity hierarchy
Instantiate prefab into scene
Instantiated entities get new UUIDs
Prefab hierarchy is preserved
PrefabComponent is added
Transform override applies
Name override applies
Active override applies
Hockey marker override applies
```

## 24.9 Component Metadata Tests

Test:

```text
ComponentRegistry registers components
FindByName works
All returns components
Transform metadata has position/rotation/scale fields
Team enum metadata exists
PlayerRole enum metadata exists
Add callback works
Remove callback works
Has callback works
Read-only metadata works for ID if exposed
```

## 24.10 Scene Lifecycle Tests

Test:

```text
OnRuntimeStart calls systems
OnRuntimeStop calls systems
OnSimulationStart works
OnSimulationStop works
OnUpdate calls systems
OnFixedUpdate calls systems
```

## 24.11 Scene Event Tests

Test:

```text
entity created event
entity destroyed event
component added event
component removed event
parent changed event
renamed event
active changed event
ClearPendingEvents clears events
```

---

# 25. Implementation Order for AI Agents

Implement in this exact order.

## Step 1 — Build and Dependency Setup

```text
1. Add entt and yaml-cpp to vcpkg.json.
2. Create libs/ecs folder.
3. Create libs/ecs/CMakeLists.txt.
4. Add hockey_ecs target.
5. Add apps/ecs_tests folder.
6. Create apps/ecs_tests/CMakeLists.txt.
7. Update root CMakeLists.txt.
8. Link client/editor/server against hockey_ecs.
9. Confirm project configures.
```

## Step 2 — Core ECS Data

```text
1. Add SceneMode.hpp.
2. Add Components.hpp.
3. Add Entity.hpp/cpp.
4. Add Scene.hpp/cpp.
5. Implement CreateEntity.
6. Implement CreateEntityWithUUID.
7. Implement entity UUID map.
8. Implement FindEntityByUUID.
9. Implement FindEntityByName.
10. Implement IsValid and ContainsUUID.
```

## Step 3 — Component API

```text
1. Implement Entity::AddComponent.
2. Implement Entity::AddOrReplaceComponent.
3. Implement Entity::GetComponent.
4. Implement Entity::HasComponent.
5. Implement Entity::RemoveComponent.
6. Add restrictions for ID/Transform/Children removal.
7. Add component events.
8. Add component tests.
```

## Step 4 — Transforms and Hierarchy

```text
1. Add Transform.hpp/cpp.
2. Implement ComposeTransform.
3. Implement DecomposeTransform.
4. Implement TransformComponent::LocalMatrix.
5. Add Hierarchy.hpp/cpp.
6. Implement SetParent.
7. Implement RemoveParent.
8. Implement IsDescendantOf.
9. Implement GetChildren.
10. Implement GetRootEntities.
11. Implement GetWorldTransform.
12. Implement SetWorldTransform.
13. Implement active state propagation.
14. Add transform/hierarchy/active tests.
```

## Step 5 — Scene Lifecycle and Events

```text
1. Add SceneEvents.hpp/cpp.
2. Implement event queue in Scene.
3. Add System.hpp/cpp.
4. Add AddSystem and ClearSystems.
5. Implement lifecycle calls.
6. Add lifecycle/event tests.
```

## Step 6 — YAML and Serialization

```text
1. Add YAML.hpp/cpp.
2. Implement vec2/vec3/vec4 YAML helpers.
3. Implement Team string conversion.
4. Implement PlayerRole string conversion.
5. Add ComponentSerializer.hpp/cpp.
6. Serialize core components.
7. Deserialize core components.
8. Serialize hockey marker components.
9. Deserialize hockey marker components.
10. Add SceneSerializer.hpp/cpp.
11. Implement scene save.
12. Implement scene load.
13. Add serialization tests.
```

## Step 7 — Validation

```text
1. Add SceneValidator.hpp/cpp.
2. Implement validation issue types.
3. Implement duplicate/missing/broken hierarchy checks.
4. Implement hockey marker validation warnings.
5. Add validation tests.
```

## Step 8 — Metadata

```text
1. Add ComponentMetadata.hpp/cpp.
2. Add ComponentRegistry.hpp/cpp.
3. Implement field metadata.
4. Implement component metadata.
5. Register Phase 2 components.
6. Add metadata tests.
```

## Step 9 — Prefabs

```text
1. Add Prefab.hpp/cpp.
2. Add PrefabSerializer.hpp/cpp.
3. Add PrefabOverride.hpp/cpp.
4. Implement SaveFromEntity.
5. Implement Instantiate.
6. Implement UUID remapping.
7. Add PrefabComponent during instancing.
8. Implement overrides.
9. Add prefab tests.
```

## Step 10 — App Integration

```text
1. Update client config with startup_scene.
2. Update editor config with startup_scene/autosave settings.
3. Update server config with startup_scene/validate_on_load.
4. Add data/raw/scenes/main_rink.scene.yaml.
5. Update GameClientApp to load Play scene.
6. Update MapEditorApp to load Edit scene.
7. Add --scene override to editor.
8. Update DedicatedServerApp to load Server scene.
9. Validate server scene on load.
10. Confirm apps run.
```

## Step 11 — Final Verification

```text
1. Build Debug Windows.
2. Build Release Windows.
3. Build Debug Linux.
4. Build Release Linux.
5. Run HockeyCoreTests.
6. Run HockeyECSTests.
7. Run client.
8. Run editor.
9. Run server.
10. Confirm no renderer/physics/networking/gameplay code exists.
```

---

# 26. Verification Commands

## Windows

```powershell
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\build\windows-debug\apps\core_tests\HockeyCoreTests.exe --root .
.\build\windows-debug\apps\ecs_tests\HockeyECSTests.exe --root .
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
./scripts/linux/run_client.sh
./scripts/linux/run_editor.sh
./scripts/linux/run_server.sh
```

---

# 27. Phase 2 Definition of Done

Phase 2 is complete only when all of this is true:

```text
entt added through vcpkg
yaml-cpp added through vcpkg
hockey_ecs builds
HockeyECSTests builds
HockeyECSTests pass
HockeyGameClient links hockey_ecs
HockeyMapEditor links hockey_ecs
HockeyDedicatedServer links hockey_ecs
entity wrapper works
scene class works
UUID map works
create entity works
create entity with UUID works
destroy entity works
destroy recursive works
duplicate entity works
component add/get/remove works
component restrictions work
hierarchy works
parent/child sync works
world/local transforms work
reparent keep world transform works
active state propagation works
scene lifecycle works
scene systems work
scene events work
YAML helpers work
component serialization works
scene save works
scene load works
scene validation works
hockey marker components serialize
component metadata registry works
prefab save works
prefab instantiate works
prefab UUID remapping works
prefab overrides work
default main_rink.scene.yaml exists
client loads scene in Play mode
editor loads scene in Edit mode
server loads scene in Server mode
server validates scene headlessly
client does not save scenes by default
no Vulkan code exists
no renderer implementation exists
no physics code exists
no networking code exists
no hockey gameplay simulation exists
no ImGui/editor UI exists
```

---

# 28. AI Completion Instruction

When Phase 2 is complete, AI agent should report:

```text
Phase 2 complete.

Implemented:
- hockey_ecs
- HockeyECSTests
- entity wrapper
- scene system
- scene modes
- core components
- hockey marker components
- hierarchy
- local/world transform system
- active state propagation
- scene lifecycle
- system interface
- scene events
- YAML helpers
- component serialization
- scene serialization
- scene validation
- component metadata registry
- prefab save/load
- prefab instancing
- prefab overrides
- default main_rink.scene.yaml
- client/editor/server scene integration

Verified:
- ECS tests
- client scene load
- editor scene load/save path
- server headless scene load/validation
- no future-phase systems were implemented
```
