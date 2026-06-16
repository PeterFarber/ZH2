# Phase 5 Plan — Complete Asset Pipeline

File path recommendation:

```text
.cursor/plans/phase-05-asset-pipeline.md
```

Cursor instruction:

```text
Read `.cursor/plans/phase-05-asset-pipeline.md` and implement Phase 5 exactly. Build the complete asset import, registry, cooking, loading, hot-reload, and editor integration pipeline. Do not implement physics simulation, networking, GameNetworkingSockets, hockey gameplay simulation, animation runtime, audio runtime, or final game UI during this phase. Keep Windows/Linux compatibility and keep the dedicated server headless.
```

---

# 0. Phase Summary

Phase 5 builds the complete asset pipeline.

This phase turns the project from using mostly built-in renderer resources and raw scene/prefab files into a real content pipeline with:

```text
asset UUIDs / asset IDs
asset handles
asset metadata
asset database
raw asset discovery
importers
cookers
cooked asset generation
runtime asset loading
editor Project panel integration
drag/drop asset assignment
shader cooking
mesh cooking
texture cooking
material assets
scene/prefab asset registration
hot reload
dependency tracking
missing reference validation
command-line asset tool
```

Phase 5 must produce:

```text
hockey_assets
HockeyAssetTool
HockeyAssetTests
```

Phase 5 updates:

```text
hockey_renderer
hockey_editor
hockey_ecs
HockeyGameClient
HockeyMapEditor
HockeyDedicatedServer only for non-GPU asset metadata/loading where needed
data/raw
data/cooked
data/config
scripts if needed
```

Phase 5 must not implement:

```text
physics simulation
Jolt integration
networking
GameNetworkingSockets
hockey gameplay simulation
animation runtime/state machine
audio runtime
final game UI
```

---

# 1. Architecture Rules

## 1.1 Dependency Direction

Add:

```text
hockey_assets -> hockey_core
```

Update renderer:

```text
hockey_renderer -> hockey_core, hockey_ecs, hockey_assets
```

Update editor:

```text
hockey_editor -> hockey_core, hockey_ecs, hockey_renderer, hockey_assets
```

Allowed app links:

```text
HockeyGameClient       -> hockey_assets
HockeyMapEditor        -> hockey_assets
HockeyAssetTool        -> hockey_assets
HockeyAssetTests       -> hockey_assets
```

Dedicated server may depend on `hockey_assets` only for CPU-side, non-rendering data:

```text
scene asset metadata
prefab asset metadata
future game-data assets
cooked YAML/binary data that does not require GPU resources
```

Dedicated server must not depend on:

```text
hockey_renderer
hockey_editor
Vulkan
ImGui
GPU texture upload paths
GPU mesh upload paths
```

## 1.2 Asset System Ownership

`hockey_assets` owns:

```text
AssetID
AssetHandle
AssetMetadata
AssetDatabase
AssetRegistry
AssetManager
raw file discovery
metadata sidecars
importer framework
cooker framework
runtime CPU-side asset loading
cooked asset paths
asset dependency graph
asset validation
hot reload events
asset watcher
command-line asset operations
```

`hockey_assets` does not own:

```text
Vulkan object creation
GPU resource lifetime
editor UI panels themselves
physics simulation
network transport
gameplay rules
audio playback
animation graph runtime
```

Renderer owns GPU copies of asset data.

Editor owns UI and workflow, but uses `hockey_assets` for database/import/cook/reload.

---

# 2. Dependencies

Update `vcpkg.json`.

Recommended additions:

```json
"fastgltf",
"ktx",
"meshoptimizer",
"stb",
"basisu"
```

If `basisu` is awkward on the current platform/toolchain, isolate it behind the texture cooker and support KTX2/uncompressed cooked textures first. The texture cooker interface must still allow later compression upgrades.

Do not add:

```text
Jolt
GameNetworkingSockets
FMOD/Wwise
extra editor UI deps beyond Phase 4
```

---

# 3. Target Project Structure

Add:

```text
libs/
  assets/
    CMakeLists.txt
    include/Hockey/Assets/
      Asset.hpp
      AssetID.hpp
      AssetHandle.hpp
      AssetType.hpp
      AssetMetadata.hpp
      AssetDatabase.hpp
      AssetRegistry.hpp
      AssetManager.hpp
      AssetPath.hpp
      AssetDependency.hpp
      AssetReference.hpp
      AssetEvent.hpp
      AssetWatcher.hpp
      Importer.hpp
      ImportContext.hpp
      ImportResult.hpp
      Cooker.hpp
      CookContext.hpp
      CookResult.hpp
      Assets/
        TextureAsset.hpp
        MeshAsset.hpp
        MaterialAsset.hpp
        ModelAsset.hpp
        ShaderAsset.hpp
        SceneAsset.hpp
        PrefabAsset.hpp
      Importers/
        TextureImporter.hpp
        GltfImporter.hpp
        MaterialImporter.hpp
        ShaderImporter.hpp
        SceneImporter.hpp
        PrefabImporter.hpp
      Cookers/
        TextureCooker.hpp
        MeshCooker.hpp
        MaterialCooker.hpp
        ShaderCooker.hpp
        SceneCooker.hpp
        PrefabCooker.hpp
      Runtime/
        AssetLoader.hpp
        TextureLoader.hpp
        MeshLoader.hpp
        MaterialLoader.hpp
        ShaderLoader.hpp
      Serialization/
        AssetMetadataSerializer.hpp
        AssetDatabaseSerializer.hpp
        MaterialSerializer.hpp
    src/
      Asset.cpp
      AssetID.cpp
      AssetHandle.cpp
      AssetType.cpp
      AssetMetadata.cpp
      AssetDatabase.cpp
      AssetRegistry.cpp
      AssetManager.cpp
      AssetPath.cpp
      AssetDependency.cpp
      AssetReference.cpp
      AssetEvent.cpp
      AssetWatcher.cpp
      Importer.cpp
      ImportContext.cpp
      ImportResult.cpp
      Cooker.cpp
      CookContext.cpp
      CookResult.cpp
      Assets/
        TextureAsset.cpp
        MeshAsset.cpp
        MaterialAsset.cpp
        ModelAsset.cpp
        ShaderAsset.cpp
        SceneAsset.cpp
        PrefabAsset.cpp
      Importers/
        TextureImporter.cpp
        GltfImporter.cpp
        MaterialImporter.cpp
        ShaderImporter.cpp
        SceneImporter.cpp
        PrefabImporter.cpp
      Cookers/
        TextureCooker.cpp
        MeshCooker.cpp
        MaterialCooker.cpp
        ShaderCooker.cpp
        SceneCooker.cpp
        PrefabCooker.cpp
      Runtime/
        AssetLoader.cpp
        TextureLoader.cpp
        MeshLoader.cpp
        MaterialLoader.cpp
        ShaderLoader.cpp
      Serialization/
        AssetMetadataSerializer.cpp
        AssetDatabaseSerializer.cpp
        MaterialSerializer.cpp

apps/
  asset_tool/
    CMakeLists.txt
    src/
      AssetToolMain.cpp
      AssetToolCommands.hpp
      AssetToolCommands.cpp

  asset_tests/
    CMakeLists.txt
    src/
      AssetTestsMain.cpp
      Test.hpp
      AssetIDTests.cpp
      AssetMetadataTests.cpp
      AssetDatabaseTests.cpp
      AssetPathTests.cpp
      TextureImportTests.cpp
      GltfImportTests.cpp
      MaterialTests.cpp
      ShaderCookTests.cpp
      DependencyTests.cpp
      HotReloadTests.cpp

data/
  raw/
    models/
    textures/
    materials/
    shaders/
    scenes/
    prefabs/
  cooked/
    asset_database.yaml
    assets/
      textures/
      meshes/
      materials/
      shaders/
      scenes/
      prefabs/
  temp/
    asset_import/
    asset_cook/
```

---

# 4. CMake Requirements

## 4.1 Root CMake

Add:

```cmake
add_subdirectory(libs/assets)
add_subdirectory(apps/asset_tool)

if(HOCKEY_BUILD_TESTS)
    add_subdirectory(apps/asset_tests)
endif()
```

Ordering should be:

```text
core
ecs
assets
renderer
editor
apps
```

## 4.2 `hockey_assets`

Create target:

```text
hockey_assets
```

Required links:

```text
hockey_core
fastgltf
KTX if used
meshoptimizer
stb include target
basisu if available
yaml-cpp if asset database/metadata use YAML
```

`hockey_assets` must not link:

```text
hockey_renderer
hockey_editor
hockey_physics
hockey_networking
hockey_gameplay
```

## 4.3 Renderer Update

`hockey_renderer` may link `hockey_assets` and consume:

```text
TextureAsset
MeshAsset
MaterialAsset
ShaderAsset
```

Renderer still owns GPU upload and GPU resource lifetime.

## 4.4 Editor Update

`hockey_editor` may link `hockey_assets` for:

```text
Project panel
asset assignment
asset status display
material editing
asset validation
hot reload
```

## 4.5 Asset Tool

Create:

```text
HockeyAssetTool
```

Links:

```text
hockey_core
hockey_assets
```

No renderer/editor dependency.

## 4.6 Asset Tests

Create:

```text
HockeyAssetTests
```

Links:

```text
hockey_core
hockey_assets
```

---

# 5. Asset Identity

## 5.1 AssetID

Files:

```text
AssetID.hpp/cpp
```

Implement:

```cpp
class AssetID {
public:
    AssetID();
    explicit AssetID(uint64_t value);

    uint64_t Value() const;
    bool IsValid() const;
    std::string ToString() const;

    bool operator==(const AssetID& other) const = default;
    bool operator!=(const AssetID& other) const = default;

private:
    uint64_t m_Value = 0;
};
```

Add `std::hash<AssetID>`.

Rules:

```text
0 means invalid.
New imported assets get nonzero IDs.
IDs persist across editor restarts.
IDs persist if the asset is moved/renamed with its metadata sidecar.
IDs are stored in sidecar metadata and asset database.
```

## 5.2 AssetHandle

Files:

```text
AssetHandle.hpp/cpp
```

Implement:

```cpp
template<typename T>
class AssetHandle {
public:
    AssetHandle() = default;
    explicit AssetHandle(AssetID id);

    AssetID ID() const;
    bool IsValid() const;

private:
    AssetID m_ID;
};
```

Aliases:

```cpp
using TextureAssetHandle = AssetHandle<TextureAsset>;
using MeshAssetHandle = AssetHandle<MeshAsset>;
using MaterialAssetHandle = AssetHandle<MaterialAsset>;
using ModelAssetHandle = AssetHandle<ModelAsset>;
using ShaderAssetHandle = AssetHandle<ShaderAsset>;
using SceneAssetHandle = AssetHandle<SceneAsset>;
using PrefabAssetHandle = AssetHandle<PrefabAsset>;
```

---

# 6. Asset Types and Metadata

## 6.1 AssetType

Files:

```text
AssetType.hpp/cpp
```

Implement:

```cpp
enum class AssetType {
    Unknown,
    Texture,
    Mesh,
    Material,
    Model,
    Shader,
    Scene,
    Prefab,
    Audio,
    Animation
};
```

Audio and Animation are future-facing types only in this phase.

Implement string conversions:

```cpp
std::string AssetTypeToString(AssetType type);
AssetType AssetTypeFromString(const std::string& value);
```

## 6.2 AssetMetadata

Files:

```text
AssetMetadata.hpp/cpp
```

Implement:

```cpp
struct AssetMetadata {
    AssetID id;
    AssetType type = AssetType::Unknown;

    std::filesystem::path rawPath;
    std::filesystem::path cookedPath;
    std::filesystem::path metadataPath;

    std::string name;
    std::string importerVersion;
    std::string cookerVersion;

    uint64_t rawFileTimestamp = 0;
    uint64_t rawFileHash = 0;
    uint64_t importedTimestamp = 0;
    uint64_t cookedTimestamp = 0;

    std::vector<AssetID> dependencies;
    std::vector<AssetID> dependents;

    bool dirty = true;
    bool missing = false;
    bool cooked = false;
};
```

Rules:

```text
Every registered asset has metadata.
Raw paths should be project-relative when stored.
Cooked paths should be project-relative when stored.
Asset ID is stable.
Hash/timestamp detects changes.
Missing raw files set missing=true.
Dirty assets need recook.
Dependencies are tracked by AssetID.
```

## 6.3 Metadata Sidecars

Use:

```text
<asset_filename>.meta.yaml
```

Example:

```yaml
Asset:
  ID: 12345678901234
  Type: Texture
  Name: ice_base_color
  ImporterVersion: TextureImporter_v1
  CookerVersion: TextureCooker_v1
  RawPath: data/raw/textures/ice_base_color.png
  CookedPath: data/cooked/assets/textures/12345678901234.ktx2
  Dependencies: []
```

---

# 7. Asset Database

## 7.1 Files

```text
AssetDatabase.hpp/cpp
Serialization/AssetDatabaseSerializer.hpp/cpp
```

Database path:

```text
data/cooked/asset_database.yaml
```

## 7.2 API

Implement:

```cpp
class AssetDatabase {
public:
    Status Load(const std::filesystem::path& path);
    Status Save(const std::filesystem::path& path) const;

    bool Contains(AssetID id) const;
    bool ContainsPath(const std::filesystem::path& rawPath) const;

    AssetMetadata* Find(AssetID id);
    const AssetMetadata* Find(AssetID id) const;

    AssetMetadata* FindByRawPath(const std::filesystem::path& rawPath);
    const AssetMetadata* FindByRawPath(const std::filesystem::path& rawPath) const;

    AssetID GetOrCreateIDForPath(const std::filesystem::path& rawPath, AssetType type);

    void AddOrUpdate(const AssetMetadata& metadata);
    void Remove(AssetID id);

    std::vector<AssetMetadata*> All();
    std::vector<const AssetMetadata*> All() const;

    std::vector<AssetMetadata*> FindByType(AssetType type);
    std::vector<AssetMetadata*> DirtyAssets();
    std::vector<AssetMetadata*> MissingAssets();

    void MarkDirty(AssetID id);
    void MarkMissing(AssetID id, bool missing);

    void RebuildDependencyGraph();
};
```

## 7.3 Requirements

```text
load/save stable database
track all discovered supported assets
preserve IDs across runs
detect moved/renamed files where metadata sidecar moves with raw file
mark deleted files as missing
track dirty assets
track dependencies and dependents
```

---

# 8. Asset Registry, Manager, Events, Watcher

## 8.1 AssetRegistry

Files:

```text
AssetRegistry.hpp/cpp
```

Purpose:

```text
Runtime map of loaded AssetID -> loaded asset object.
```

API:

```cpp
class AssetRegistry {
public:
    template<typename T>
    std::shared_ptr<T> Get(AssetID id);

    template<typename T>
    void Store(AssetID id, std::shared_ptr<T> asset);

    void Unload(AssetID id);
    void UnloadAll();

    bool IsLoaded(AssetID id) const;
};
```

## 8.2 AssetManager

Files:

```text
AssetManager.hpp/cpp
```

API:

```cpp
struct AssetManagerCreateInfo {
    std::filesystem::path rawRoot;
    std::filesystem::path cookedRoot;
    std::filesystem::path databasePath;
};

class AssetManager {
public:
    Status Init(const AssetManagerCreateInfo& info);
    void Shutdown();

    AssetDatabase& Database();
    const AssetDatabase& Database() const;

    Status DiscoverRawAssets();
    Status ImportAsset(const std::filesystem::path& rawPath);
    Status ImportAll();
    Status CookAsset(AssetID id);
    Status CookAllDirty();
    Status RecookAll();
    Status ValidateReferences();

    template<typename T>
    Result<std::shared_ptr<T>> Load(AssetID id);

    void Unload(AssetID id);
    void UnloadAll();

    const std::vector<AssetEvent>& PollEvents();
};
```

## 8.3 AssetEvent

Files:

```text
AssetEvent.hpp/cpp
```

Implement:

```cpp
enum class AssetEventType {
    Imported,
    Cooked,
    Modified,
    Deleted,
    Moved,
    Reloaded,
    Failed
};

struct AssetEvent {
    AssetEventType type;
    AssetID id;
    std::filesystem::path path;
    std::string message;
};
```

## 8.4 AssetWatcher

Files:

```text
AssetWatcher.hpp/cpp
```

Use polling for cross-platform reliability.

API:

```cpp
class AssetWatcher {
public:
    void Start(const std::filesystem::path& rawRoot);
    void Stop();

    std::vector<std::filesystem::path> PollChangedFiles();
};
```

Must detect:

```text
new files
modified files
deleted files
```

---

# 9. Importer and Cooker Framework

## 9.1 Importer

Files:

```text
Importer.hpp/cpp
ImportContext.hpp/cpp
ImportResult.hpp/cpp
```

Implement:

```cpp
struct ImportContext {
    std::filesystem::path rawPath;
    std::filesystem::path rawRoot;
    std::filesystem::path cookedRoot;
    AssetDatabase* database = nullptr;
};

struct ImportResult {
    bool success = false;
    std::string error;
    AssetMetadata metadata;
    std::vector<AssetMetadata> generatedAssets;
};

class Importer {
public:
    virtual ~Importer() = default;

    virtual bool SupportsExtension(const std::string& extension) const = 0;
    virtual AssetType GetAssetType() const = 0;
    virtual ImportResult Import(const ImportContext& context) = 0;
};
```

## 9.2 Cooker

Files:

```text
Cooker.hpp/cpp
CookContext.hpp/cpp
CookResult.hpp/cpp
```

Implement:

```cpp
struct CookContext {
    AssetMetadata metadata;
    std::filesystem::path rawRoot;
    std::filesystem::path cookedRoot;
    AssetDatabase* database = nullptr;
};

struct CookResult {
    bool success = false;
    std::string error;
    std::filesystem::path cookedPath;
    std::vector<AssetID> dependencies;
};

class Cooker {
public:
    virtual ~Cooker() = default;

    virtual AssetType GetAssetType() const = 0;
    virtual CookResult Cook(const CookContext& context) = 0;
};
```

## 9.3 Registration

AssetManager registers:

```text
TextureImporter + TextureCooker
GltfImporter + Mesh/Model/Material generation
MaterialImporter + MaterialCooker
ShaderImporter + ShaderCooker
SceneImporter + SceneCooker
PrefabImporter + PrefabCooker
```

---

# 10. Asset Type Pipelines

## 10.1 Texture Pipeline

Files:

```text
TextureAsset.hpp/cpp
TextureImporter.hpp/cpp
TextureCooker.hpp/cpp
TextureLoader.hpp/cpp
```

Supported raw formats:

```text
.png
.jpg
.jpeg
.tga
.bmp
.hdr
.ktx
.ktx2
```

Settings:

```cpp
enum class TextureColorSpace { Linear, SRGB };
enum class TextureSemantic { Unknown, BaseColor, Normal, MetallicRoughness, Occlusion, Emissive, Height, Mask };

struct TextureImportSettings {
    TextureColorSpace colorSpace = TextureColorSpace::SRGB;
    TextureSemantic semantic = TextureSemantic::Unknown;
    bool generateMipmaps = true;
    bool compress = true;
    bool normalMap = false;
    int maxSize = 4096;
};
```

Runtime asset:

```cpp
struct TextureAsset {
    AssetID id;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    TextureColorSpace colorSpace = TextureColorSpace::SRGB;
    TextureSemantic semantic = TextureSemantic::Unknown;
    std::vector<std::byte> data;
};
```

## 10.2 Mesh / Model / glTF Pipeline

Files:

```text
MeshAsset.hpp/cpp
ModelAsset.hpp/cpp
GltfImporter.hpp/cpp
MeshCooker.hpp/cpp
MeshLoader.hpp/cpp
```

Supported:

```text
.gltf
.glb
```

Mesh data:

```cpp
struct MeshVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec2 uv0;
};

struct MeshSubmesh {
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
    AssetID materialId;
};

struct MeshAsset {
    AssetID id;
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MeshSubmesh> submeshes;
    glm::vec3 boundsMin;
    glm::vec3 boundsMax;
};
```

Cooker requirements:

```text
extract vertices/indices/submeshes
compute bounds
preserve material references
optimize vertex cache with meshoptimizer
optimize vertex fetch where appropriate
write cooked mesh file
```

## 10.3 Material Pipeline

Files:

```text
MaterialAsset.hpp/cpp
MaterialImporter.hpp/cpp
MaterialCooker.hpp/cpp
MaterialLoader.hpp/cpp
MaterialSerializer.hpp/cpp
```

Raw material format:

```text
.material.yaml
```

Example:

```yaml
Material:
  Name: Ice
  Type: PBR
  BaseColor: [0.75, 0.9, 1.0, 1.0]
  Metallic: 0.0
  Roughness: 0.18
  NormalStrength: 1.0
  AlphaMode: Opaque
  Textures:
    BaseColor: data/raw/textures/ice_base_color.png
    Normal: data/raw/textures/ice_normal.png
```

Material data:

```cpp
struct MaterialAsset {
    AssetID id;
    std::string name;

    glm::vec4 baseColor = {1,1,1,1};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float normalStrength = 1.0f;
    float occlusionStrength = 1.0f;
    glm::vec3 emissiveColor{0.0f};
    float emissiveStrength = 0.0f;

    AssetID baseColorTexture;
    AssetID normalTexture;
    AssetID metallicRoughnessTexture;
    AssetID occlusionTexture;
    AssetID emissiveTexture;

    std::string alphaMode = "Opaque";
    float alphaCutoff = 0.5f;
};
```

## 10.4 Shader Pipeline

Files:

```text
ShaderAsset.hpp/cpp
ShaderImporter.hpp/cpp
ShaderCooker.hpp/cpp
ShaderLoader.hpp/cpp
```

Supported:

```text
.glsl
.vert
.frag
.comp
```

Cook requirements:

```text
compile GLSL to SPIR-V
support include directory
support defines/variants where practical
write .spv to cooked shader folder
report compile errors
track shader include dependencies
```

## 10.5 Scene and Prefab Asset Tracking

Files:

```text
SceneAsset.hpp/cpp
PrefabAsset.hpp/cpp
SceneImporter.hpp/cpp
PrefabImporter.hpp/cpp
SceneCooker.hpp/cpp
PrefabCooker.hpp/cpp
```

Track:

```text
.scene.yaml
.prefab.yaml
dependencies on materials/meshes/textures/prefabs
```

Cooked scene/prefab can initially be validated copied YAML or cooked binary, but it must be represented as a cooked asset and tracked in the database.

---

# 11. Cooked File Formats

All cooked assets live under:

```text
data/cooked/assets/
```

Suggested paths:

```text
data/cooked/assets/textures/<asset_id>.ktx2
data/cooked/assets/meshes/<asset_id>.mesh.bin
data/cooked/assets/materials/<asset_id>.material.yaml or .material.bin
data/cooked/assets/shaders/<asset_id>.spv
data/cooked/assets/scenes/<asset_id>.scene.yaml or .scene.bin
data/cooked/assets/prefabs/<asset_id>.prefab.yaml or .prefab.bin
```

Every cooked format must include or associate:

```text
asset type
asset ID
format version
source hash
cooker version
```

Binary header:

```cpp
struct CookedAssetHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t assetType;
    uint64_t assetID;
    uint64_t sourceHash;
};
```

---

# 12. ECS/Renderer Asset References

Update `MeshRendererComponent` to support AssetID references while keeping built-in fallback names:

```cpp
struct MeshRendererComponent {
    AssetID meshAsset;
    AssetID materialAsset;

    std::string builtInMeshName;
    std::string builtInMaterialName;

    bool visible = true;
    bool castsShadows = true;
    bool receivesShadows = true;
};
```

Rules:

```text
If meshAsset is valid, use asset.
Else use builtInMeshName.
If materialAsset is valid, use asset.
Else use builtInMaterialName.
```

Update:

```text
ComponentSerializer
ComponentRegistry metadata
SceneSerializer
PrefabSerializer
Inspector field drawers
```

---

# 13. Runtime Loading

Implement `AssetLoader` and type loaders:

```cpp
class AssetLoader {
public:
    Result<std::shared_ptr<TextureAsset>> LoadTexture(const AssetMetadata& metadata);
    Result<std::shared_ptr<MeshAsset>> LoadMesh(const AssetMetadata& metadata);
    Result<std::shared_ptr<MaterialAsset>> LoadMaterial(const AssetMetadata& metadata);
    Result<std::shared_ptr<ShaderAsset>> LoadShader(const AssetMetadata& metadata);
};
```

Rules:

```text
load cooked files during runtime
return Result<T> on failure
log missing/corrupt cooked assets
cache loaded assets in AssetRegistry
renderer creates GPU resources from loaded CPU asset data
```

Renderer integration:

```text
create GPU mesh from MeshAsset
create GPU texture from TextureAsset
create GPU material from MaterialAsset
create shader module from ShaderAsset
reload GPU resource on asset reload event where possible
```

---

# 14. Editor Integration

## 14.1 Project Panel

Update Project Panel to use `AssetManager` and `AssetDatabase`.

Show:

```text
asset name
asset type
asset ID
raw path
cooked path
status: cooked/dirty/missing/unsupported
```

Context actions:

```text
Import
Reimport
Cook
Recook
Recook All Dirty
Reveal in Explorer/File Manager
Copy Asset ID
Delete Metadata
Validate References
```

## 14.2 Inspector Asset Fields

Support asset fields:

```text
AssetID
TextureAsset
MeshAsset
MaterialAsset
ShaderAsset
SceneAsset
PrefabAsset
```

Behavior:

```text
drag asset into compatible field
clear asset field
show asset name/path
show missing asset warning
open asset in Project Panel
```

## 14.3 Drag/Drop Workflows

Implement:

```text
drag MeshAsset onto entity -> set MeshRendererComponent meshAsset
drag MaterialAsset onto entity -> set MeshRendererComponent materialAsset
drag PrefabAsset into hierarchy -> instantiate prefab
drag PrefabAsset into viewport -> instantiate prefab
drag SceneAsset -> prompt to open scene
drag TextureAsset onto MaterialAsset field -> assign texture
```

## 14.4 Material Editing

Add material editing support:

```text
create material asset
edit base color
edit metallic
edit roughness
edit normal strength
assign textures
save material asset
cook material asset
preview material on selected entity
```

---

# 15. Hot Reload

Editor should:

```text
poll AssetWatcher
detect modified raw assets
mark assets dirty
auto-import if enabled
auto-cook if enabled
emit AssetEvent
notify editor panels
notify renderer
```

Renderer should:

```text
reload texture GPU resource when TextureAsset changes
reload material data when MaterialAsset changes
reload mesh GPU resource when MeshAsset changes if feasible
reload shader/pipeline when ShaderAsset changes if feasible
fall back safely on reload failure
```

Editor config:

```toml
[assets]
auto_discover = true
auto_import = true
auto_cook_dirty = true
hot_reload = true
```

---

# 16. Dependency Tracking

Track dependencies for:

```text
MaterialAsset -> TextureAsset references
ModelAsset -> MeshAsset/MaterialAsset/TextureAsset references
SceneAsset -> PrefabAsset/MaterialAsset/MeshAsset/TextureAsset references
PrefabAsset -> MaterialAsset/MeshAsset/TextureAsset references
ShaderAsset -> included shader files
```

When dependency changes:

```text
mark dependents dirty
recook dependents as needed
```

Validation detects:

```text
missing dependency
asset ID not in database
raw file missing
cooked file missing
unsupported extension
stale cooked asset
invalid/cyclic dependency where applicable
```

---

# 17. Import Rules

Supported extensions:

```text
Texture:  .png .jpg .jpeg .tga .bmp .hdr .ktx .ktx2
Model:    .gltf .glb
Material: .material.yaml
Shader:   .glsl .vert .frag .comp
Scene:    .scene.yaml
Prefab:   .prefab.yaml
```

Unsupported files:

```text
show as unsupported in editor
ignore during import-all unless explicitly imported
do not fail discovery
```

Implement stable file hash:

```text
64-bit hash of file contents
```

Use hash for:

```text
dirty detection
source hash in cooked output
recook decisions
```

---

# 18. Config Updates

## 18.1 `client.toml`

Add:

```toml
[assets]
raw_root = "data/raw"
cooked_root = "data/cooked"
database = "data/cooked/asset_database.yaml"
load_from_cooked = true
```

## 18.2 `editor.toml`

Add:

```toml
[assets]
raw_root = "data/raw"
cooked_root = "data/cooked"
database = "data/cooked/asset_database.yaml"
auto_discover = true
auto_import = true
auto_cook_dirty = true
hot_reload = true
```

## 18.3 `server.toml`

Add only if the server needs asset metadata/non-GPU cooked data:

```toml
[assets]
raw_root = "data/raw"
cooked_root = "data/cooked"
database = "data/cooked/asset_database.yaml"
load_from_cooked = true
load_gpu_assets = false
```

---

# 19. Asset Tool

Create:

```text
HockeyAssetTool
```

Supported commands:

```text
HockeyAssetTool --root . discover
HockeyAssetTool --root . import data/raw/textures/ice.png
HockeyAssetTool --root . import-all
HockeyAssetTool --root . cook <asset-id>
HockeyAssetTool --root . cook-all-dirty
HockeyAssetTool --root . recook-all
HockeyAssetTool --root . validate
HockeyAssetTool --root . list
HockeyAssetTool --root . print <asset-id>
```

Output examples:

```text
[OK] Imported Texture data/raw/textures/ice.png -> AssetID 123
[WARN] Dirty Material data/raw/materials/ice.material.yaml
[ERROR] Missing dependency AssetID 456 referenced by material 123
```

Commands:

```text
discover: scan data/raw, create/update metadata, save database
import: import one raw file, save database
import-all: import all supported raw files, save database
cook: cook one asset, update status
cook-all-dirty: cook dirty assets and dependent assets
recook-all: force recook all supported assets
validate: check missing files/cooked outputs/dependencies/references
list: print asset ID, type, status, path
print: print metadata for one asset
```

---

# 20. Tests

Create:

```text
HockeyAssetTests
```

Use simple test harness.

## 20.1 AssetIDTests

```text
generated ID nonzero
IDs differ
ID string non-empty
hash map works
invalid ID is 0
```

## 20.2 AssetMetadataTests

```text
metadata serializes
metadata deserializes
dependencies preserved
dirty/missing/cooked flags preserved
paths preserved
```

## 20.3 AssetDatabaseTests

```text
load empty database
add asset
find by ID
find by raw path
save database
reload database
mark dirty
mark missing
remove asset
find by type
dirty asset list
missing asset list
dependency graph rebuild
```

## 20.4 AssetPathTests

```text
project-relative path conversion
path normalization
raw root detection
cooked path generation
metadata sidecar path generation
Windows-like path handling where possible
Linux path handling
```

## 20.5 TextureImportTests

```text
import PNG test file
create metadata
detect color space setting
cook texture
load cooked texture
missing texture reports error
```

## 20.6 GltfImportTests

```text
import minimal glTF/GLB fixture
extract mesh metadata
extract material metadata
detect texture dependencies
cook mesh
load cooked mesh
```

## 20.7 MaterialTests

```text
load material YAML
parse PBR fields
parse texture references
detect dependencies
cook material
load cooked material
missing texture dependency warning/error
```

## 20.8 ShaderCookTests

```text
discover shader
compile shader
write SPIR-V
detect compile error
track include dependency if include support implemented
```

## 20.9 DependencyTests

```text
material depends on texture
model depends on material/mesh/texture
dirty dependency marks dependent dirty
missing dependency is reported
```

## 20.10 HotReloadTests

```text
watcher detects modified temp file
asset marked dirty
asset event emitted
reload event emitted after recook
```

---

# 21. Manual Editor Validation

Verify in editor:

```text
Project panel shows asset database contents
Discover assets from menu/tool
Import texture
Import glTF
Create material
Assign texture to material
Assign mesh/material to entity
Save scene with asset IDs
Reload scene and asset references remain
Modify texture file and hot reload
Delete raw asset and missing state appears
Run recook all dirty
```

---

# 22. Implementation Order for Cursor

## Step 1 — Target and Dependency Setup

```text
1. Add asset dependencies to vcpkg.json.
2. Create libs/assets folder.
3. Create libs/assets/CMakeLists.txt.
4. Add hockey_assets target.
5. Create apps/asset_tool target.
6. Create apps/asset_tests target.
7. Update root CMakeLists.txt ordering.
8. Update renderer/editor links to hockey_assets.
9. Confirm server does not gain renderer/editor dependencies.
10. Configure/build.
```

## Step 2 — Asset Identity and Metadata

```text
1. Add AssetID.
2. Add AssetHandle.
3. Add AssetType.
4. Add AssetMetadata.
5. Add metadata serialization.
6. Add AssetIDTests.
7. Add AssetMetadataTests.
```

## Step 3 — Asset Database

```text
1. Add AssetDatabase.
2. Add AssetDatabaseSerializer.
3. Implement load/save.
4. Implement find/add/remove/dirty/missing APIs.
5. Implement dependency graph rebuild.
6. Add AssetDatabaseTests.
```

## Step 4 — Asset Manager and Registry

```text
1. Add AssetRegistry.
2. Add AssetManager.
3. Add AssetEvent.
4. Add AssetWatcher.
5. Add raw/cooked root config.
6. Add basic discover support.
```

## Step 5 — Importer/Cooker Framework

```text
1. Add Importer base.
2. Add ImportContext/ImportResult.
3. Add Cooker base.
4. Add CookContext/CookResult.
5. Register importer/cooker interfaces in AssetManager.
```

## Step 6 — Texture Pipeline

```text
1. Add TextureAsset.
2. Add TextureImporter.
3. Add TextureCooker.
4. Add TextureLoader.
5. Add texture import settings.
6. Add TextureImportTests.
```

## Step 7 — Material Pipeline

```text
1. Add MaterialAsset.
2. Add MaterialSerializer.
3. Add MaterialImporter.
4. Add MaterialCooker.
5. Add MaterialLoader.
6. Add MaterialTests.
```

## Step 8 — glTF/Mesh Pipeline

```text
1. Add MeshAsset.
2. Add ModelAsset.
3. Add GltfImporter.
4. Add MeshCooker.
5. Add MeshLoader.
6. Integrate meshoptimizer.
7. Add GltfImportTests.
```

## Step 9 — Shader Pipeline

```text
1. Add ShaderAsset.
2. Add ShaderImporter.
3. Add ShaderCooker.
4. Add ShaderLoader.
5. Integrate cooked shader path with renderer.
6. Add ShaderCookTests.
```

## Step 10 — Scene/Prefab Asset Tracking

```text
1. Add SceneAsset.
2. Add PrefabAsset.
3. Add SceneImporter/Cooker.
4. Add PrefabImporter/Cooker.
5. Track dependencies from scenes/prefabs to assets.
```

## Step 11 — Renderer Integration

```text
1. Update MeshRendererComponent to support AssetID references.
2. Update serialization/metadata for asset IDs.
3. Update renderer to load MeshAsset.
4. Update renderer to load TextureAsset.
5. Update renderer to load MaterialAsset.
6. Keep built-in fallback support.
7. Implement GPU resource reload where possible.
```

## Step 12 — Editor Integration

```text
1. Update ProjectPanel to use AssetDatabase.
2. Add asset status display.
3. Add import/cook/recook actions.
4. Add asset drag/drop.
5. Add asset field drawers.
6. Add material editing support.
7. Add missing reference warnings.
8. Add hot reload polling.
```

## Step 13 — Asset Tool

```text
1. Add command parser.
2. Implement discover.
3. Implement import.
4. Implement import-all.
5. Implement cook.
6. Implement cook-all-dirty.
7. Implement recook-all.
8. Implement validate.
9. Implement list.
10. Implement print.
```

## Step 14 — Final Verification

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
10. Run HockeyAssetTool discover/import-all/cook-all-dirty/validate.
11. Launch editor and verify asset workflows.
12. Launch client and verify cooked asset loading.
13. Launch server and verify headless behavior.
```

---

# 23. Verification Commands

## Windows

```powershell
.\scripts\windows\configure_debug.ps1
.\scripts\windows\build_debug.ps1
.\build\windows-debug\apps\core_tests\HockeyCoreTests.exe --root .
.\build\windows-debug\apps\ecs_tests\HockeyECSTests.exe --root .
.\build\windows-debug\apps\renderer_tests\HockeyRendererTests.exe --root .
.\build\windows-debug\apps\editor_tests\HockeyEditorTests.exe --root .
.\build\windows-debug\apps\asset_tests\HockeyAssetTests.exe --root .
.\build\windows-debug\apps\asset_tool\HockeyAssetTool.exe --root . discover
.\build\windows-debug\apps\asset_tool\HockeyAssetTool.exe --root . import-all
.\build\windows-debug\apps\asset_tool\HockeyAssetTool.exe --root . cook-all-dirty
.\build\windows-debug\apps\asset_tool\HockeyAssetTool.exe --root . validate
.\scripts\windows\run_editor.ps1
.\scripts\windows\run_client.ps1
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
./build/linux-debug/apps/asset_tool/HockeyAssetTool --root . discover
./build/linux-debug/apps/asset_tool/HockeyAssetTool --root . import-all
./build/linux-debug/apps/asset_tool/HockeyAssetTool --root . cook-all-dirty
./build/linux-debug/apps/asset_tool/HockeyAssetTool --root . validate
./scripts/linux/run_editor.sh
./scripts/linux/run_client.sh
./scripts/linux/run_server.sh
```

---

# 24. Phase 5 Definition of Done

Phase 5 is complete only when all of this is true:

```text
hockey_assets builds
HockeyAssetTool builds
HockeyAssetTests builds
HockeyAssetTests pass
asset dependencies added through vcpkg
asset IDs work
asset handles work
asset metadata serializes
asset database loads/saves
raw asset discovery works
metadata sidecars work
dirty detection works
missing file detection works
dependency tracking works
dependency dirty propagation works
texture import works
texture cooking works
texture runtime loading works
glTF/GLB import works
mesh cooking works
mesh runtime loading works
material YAML works
material cooking works
material runtime loading works
shader discovery works
shader cooking works
scene asset tracking works
prefab asset tracking works
HockeyAssetTool discover works
HockeyAssetTool import/import-all works
HockeyAssetTool cook/cook-all-dirty/recook-all works
HockeyAssetTool validate works
renderer loads assets through asset system
renderer keeps built-in fallbacks
editor Project panel uses asset database
editor can import/cook/recook assets
editor can assign mesh/material/texture assets
editor can show dirty/missing/cooked status
hot reload detects changed files
hot reload updates texture/material/mesh where possible
client can load cooked assets
server remains headless
server does not load GPU resources
Windows build works
Linux build works
no physics simulation exists
no networking code exists
no hockey gameplay simulation exists
no animation runtime exists
no audio runtime exists
```

---

# 25. Cursor Completion Instruction

When Phase 5 is complete, Cursor should report:

```text
Phase 5 complete.

Implemented:
- hockey_assets
- HockeyAssetTool
- HockeyAssetTests
- AssetID and AssetHandle
- AssetMetadata
- AssetDatabase
- AssetRegistry
- AssetManager
- Importer/Cooker framework
- Texture pipeline
- Material pipeline
- glTF/model/mesh pipeline
- Shader pipeline
- Scene/prefab asset tracking
- cooked asset formats
- dependency tracking
- dirty/missing validation
- hot reload watcher/events
- renderer asset loading integration
- editor Project panel asset integration
- asset field drawers and drag/drop assignment
- asset tool commands

Verified:
- asset tests
- asset tool discover/import/cook/validate
- editor asset workflow
- client cooked asset loading
- server remains headless
- Windows/Linux builds
- no future-phase systems were implemented
```
