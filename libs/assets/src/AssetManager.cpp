#include "Hockey/Assets/AssetManager.hpp"

#include "Hockey/Assets/AssetHash.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Assets/MaterialAsset.hpp"
#include "Hockey/Assets/Assets/MeshAsset.hpp"
#include "Hockey/Assets/Assets/ModelAsset.hpp"
#include "Hockey/Assets/Assets/ShaderAsset.hpp"
#include "Hockey/Assets/Assets/TextureAsset.hpp"
#include "Hockey/Assets/Cookers/MaterialCooker.hpp"
#include "Hockey/Assets/Cookers/MeshCooker.hpp"
#include "Hockey/Assets/Cookers/ModelCooker.hpp"
#include "Hockey/Assets/Cookers/PrefabCooker.hpp"
#include "Hockey/Assets/Cookers/SceneCooker.hpp"
#include "Hockey/Assets/Cookers/ShaderCooker.hpp"
#include "Hockey/Assets/Cookers/TextureCooker.hpp"
#include "Hockey/Assets/Importers/GltfImporter.hpp"
#include "Hockey/Assets/Importers/MaterialImporter.hpp"
#include "Hockey/Assets/Importers/PrefabImporter.hpp"
#include "Hockey/Assets/Importers/SceneImporter.hpp"
#include "Hockey/Assets/Importers/ShaderImporter.hpp"
#include "Hockey/Assets/Importers/TextureImporter.hpp"
#include "Hockey/Assets/Runtime/AssetLoader.hpp"
#include "Hockey/Assets/Serialization/AssetMetadataSerializer.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Log.hpp"

#include <algorithm>
#include <chrono>
#include <system_error>
#include <unordered_set>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

uint64_t NowSeconds() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

uint64_t FileTimestamp(const fs::path& path) {
    std::error_code ec;
    const auto time = fs::last_write_time(path, ec);
    if (ec) {
        return 0;
    }
    return static_cast<uint64_t>(time.time_since_epoch().count());
}

std::string LowerExtension(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

bool HasSuffix(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Generated sub-assets (e.g. meshes from a glTF) use a synthetic raw path that
// embeds a '#'. They are not directly file-backed.
bool IsGeneratedRawPath(const fs::path& rawPath) {
    return rawPath.generic_string().find('#') != std::string::npos;
}

} // namespace

AssetManager::AssetManager() = default;
AssetManager::~AssetManager() {
    Shutdown();
}

AssetManagerCreateInfo AssetManager::DefaultCreateInfo(const fs::path& projectRoot) {
    AssetManagerCreateInfo info;
    info.projectRoot = projectRoot;
    info.rawRoot = projectRoot / "data" / "raw";
    info.cookedRoot = projectRoot / "data" / "cooked";
    info.databasePath = projectRoot / "data" / "cooked" / "asset_database.yaml";
    return info;
}

AssetType AssetManager::ClassifyExtension(const fs::path& rawPath) {
    std::string name = rawPath.filename().string();
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (HasSuffix(name, ".material.yaml"))
        return AssetType::Material;
    if (HasSuffix(name, ".scene.yaml"))
        return AssetType::Scene;
    if (HasSuffix(name, ".prefab.yaml"))
        return AssetType::Prefab;

    const std::string ext = LowerExtension(rawPath);
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".hdr" ||
        ext == ".ktx" || ext == ".ktx2")
        return AssetType::Texture;
    if (ext == ".gltf" || ext == ".glb")
        return AssetType::Model;
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".comp")
        return AssetType::Shader;
    return AssetType::Unknown;
}

fs::path AssetManager::AbsoluteRaw(const fs::path& projectRelative) const {
    return AssetPath::ToAbsolute(m_Info.projectRoot, projectRelative);
}

fs::path AssetManager::ToProjectRelative(const fs::path& absolute) const {
    return AssetPath::ToProjectRelative(m_Info.projectRoot, absolute);
}

Status AssetManager::Init(const AssetManagerCreateInfo& info) {
    m_Info = info;
    FileSystem::CreateDirectories(m_Info.rawRoot);
    FileSystem::CreateDirectories(m_Info.cookedRoot);
    FileSystem::CreateDirectories(m_Info.cookedRoot / "assets");

    RegisterPipelines();

    const Status load = m_Database.Load(m_Info.databasePath);
    if (!load) {
        HK_CORE_WARN("Asset database failed to load ({}); starting empty", load.error);
        m_Database.Clear();
    }
    m_Initialized = true;
    HK_CORE_INFO("AssetManager initialized (raw='{}', cooked='{}', {} assets)", m_Info.rawRoot.generic_string(),
                 m_Info.cookedRoot.generic_string(), m_Database.Count());
    return Status::Ok();
}

void AssetManager::Shutdown() {
    if (!m_Initialized) {
        return;
    }
    StopWatching();
    m_Registry.UnloadAll();
    m_Importers.clear();
    m_Cookers.clear();
    m_Initialized = false;
}

void AssetManager::RegisterImporter(std::unique_ptr<Importer> importer) {
    m_Importers.push_back(std::move(importer));
}

void AssetManager::RegisterCooker(std::unique_ptr<Cooker> cooker) {
    m_Cookers[cooker->GetAssetType()] = std::move(cooker);
}

Importer* AssetManager::FindImporterForExtension(const std::string& extension) const {
    for (const std::unique_ptr<Importer>& importer : m_Importers) {
        if (importer->SupportsExtension(extension)) {
            return importer.get();
        }
    }
    return nullptr;
}

Importer* AssetManager::FindImporter(AssetType type, const std::string& extension) const {
    Importer* byExtension = nullptr;
    for (const std::unique_ptr<Importer>& importer : m_Importers) {
        if (importer->GetAssetType() == type) {
            return importer.get();
        }
        if (byExtension == nullptr && importer->SupportsExtension(extension)) {
            byExtension = importer.get();
        }
    }
    return byExtension;
}

Cooker* AssetManager::FindCooker(AssetType type) const {
    auto it = m_Cookers.find(type);
    return it == m_Cookers.end() ? nullptr : it->second.get();
}

Status AssetManager::DiscoverRawAssets() {
    std::error_code ec;
    if (!fs::exists(m_Info.rawRoot, ec)) {
        return Status::Ok();
    }

    std::unordered_set<AssetID> seen;
    for (auto it = fs::recursive_directory_iterator(m_Info.rawRoot, ec);
         !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) {
            continue;
        }
        const fs::path absolute = it->path();
        if (AssetPath::IsMetadataSidecar(absolute)) {
            continue;
        }
        const AssetType type = ClassifyExtension(absolute);
        if (type == AssetType::Unknown) {
            continue; // unsupported files are ignored during discovery
        }

        const fs::path projectRel = ToProjectRelative(absolute);
        const fs::path sidecarPath = AssetPath::MetadataSidecar(absolute);

        // Resolve a stable id: existing db record > sidecar id > new id.
        AssetID id;
        if (const AssetMetadata* existing = m_Database.FindByRawPath(projectRel)) {
            id = existing->id;
        } else if (FileSystem::Exists(sidecarPath)) {
            Result<AssetMetadata> sidecar = AssetMetadataSerializer::LoadSidecar(sidecarPath);
            if (sidecar && sidecar.value.id.IsValid()) {
                AssetMetadata adopted = sidecar.value;
                adopted.rawPath = projectRel;
                adopted.type = type;
                m_Database.AddOrUpdate(adopted);
                id = adopted.id;
            }
        }
        if (!id.IsValid()) {
            id = m_Database.GetOrCreateIDForPath(projectRel, type);
        }

        AssetMetadata* metadata = m_Database.Find(id);
        if (metadata == nullptr) {
            continue;
        }
        seen.insert(id);

        metadata->type = type;
        metadata->rawPath = projectRel;
        metadata->metadataPath = ToProjectRelative(sidecarPath);
        metadata->missing = false;

        const uint64_t newHash = AssetHash::HashFile(absolute);
        const uint64_t newStamp = FileTimestamp(absolute);
        if (metadata->rawFileHash != newHash || !metadata->cooked) {
            metadata->dirty = true;
        }
        metadata->rawFileHash = newHash;
        metadata->rawFileTimestamp = newStamp;
        if (metadata->name.empty()) {
            metadata->name = absolute.stem().string();
        }

        // Refresh the sidecar so new files gain one and ids persist across moves.
        AssetMetadataSerializer::SaveSidecar(sidecarPath, *metadata);
    }

    // Mark file-backed assets whose raw file disappeared as missing.
    for (AssetMetadata* metadata : m_Database.All()) {
        if (metadata->rawPath.empty() || IsGeneratedRawPath(metadata->rawPath)) {
            continue;
        }
        if (seen.find(metadata->id) == seen.end()) {
            const fs::path absolute = AbsoluteRaw(metadata->rawPath);
            if (!FileSystem::Exists(absolute)) {
                metadata->missing = true;
            }
        }
    }

    m_Database.RebuildDependencyGraph();
    return SaveDatabase();
}

Status AssetManager::ImportMetadata(AssetMetadata& metadata) {
    const fs::path absolute = AbsoluteRaw(metadata.rawPath);
    const std::string ext = LowerExtension(metadata.rawPath);

    Importer* importer = FindImporter(metadata.type, ext);
    if (importer == nullptr) {
        // No importer registered for this type yet: discovery already recorded
        // basic metadata, so this is a no-op (the asset can still be cooked).
        metadata.dirty = true;
        metadata.importedTimestamp = NowSeconds();
        return Status::Ok();
    }

    ImportContext context;
    context.rawPath = absolute;
    context.rawRoot = m_Info.rawRoot;
    context.cookedRoot = m_Info.cookedRoot;
    context.projectRoot = m_Info.projectRoot;
    context.existingId = metadata.id;
    context.database = &m_Database;

    ImportResult result = importer->Import(context);
    if (!result.success) {
        PushEvent({AssetEventType::Failed, metadata.id, metadata.rawPath, result.error});
        return Status::Fail(result.error);
    }

    // Merge importer output into the canonical record.
    metadata.type = importer->GetAssetType();
    metadata.importerVersion = importer->Version();
    if (!result.metadata.name.empty()) {
        metadata.name = result.metadata.name;
    }
    metadata.dependencies = result.metadata.dependencies;
    metadata.dirty = true;
    metadata.importedTimestamp = NowSeconds();

    // Register any generated sub-assets (meshes/materials from a model, etc.).
    for (AssetMetadata& generated : result.generatedAssets) {
        generated.dirty = true;
        generated.importedTimestamp = metadata.importedTimestamp;
        m_Database.AddOrUpdate(generated);
    }

    m_Database.AddOrUpdate(metadata);
    AssetMetadataSerializer::SaveSidecar(AbsoluteRaw(metadata.metadataPath), metadata);
    PushEvent({AssetEventType::Imported, metadata.id, metadata.rawPath, {}});
    return Status::Ok();
}

Status AssetManager::ImportAsset(const fs::path& rawPath) {
    fs::path absolute = rawPath.is_absolute() ? rawPath : AbsoluteRaw(rawPath);
    if (!FileSystem::Exists(absolute)) {
        return Status::Fail("raw file does not exist: " + absolute.string());
    }
    const AssetType type = ClassifyExtension(absolute);
    if (type == AssetType::Unknown) {
        return Status::Fail("unsupported asset extension: " + absolute.string());
    }

    const fs::path projectRel = ToProjectRelative(absolute);
    const AssetID id = m_Database.GetOrCreateIDForPath(projectRel, type);
    AssetMetadata* metadata = m_Database.Find(id);
    if (metadata == nullptr) {
        return Status::Fail("failed to register asset: " + absolute.string());
    }
    metadata->type = type;
    metadata->rawFileHash = AssetHash::HashFile(absolute);
    metadata->rawFileTimestamp = FileTimestamp(absolute);
    metadata->metadataPath = ToProjectRelative(AssetPath::MetadataSidecar(absolute));
    metadata->missing = false;

    const Status status = ImportMetadata(*metadata);
    m_Database.RebuildDependencyGraph();
    SaveDatabase();
    return status;
}

Status AssetManager::ImportAll() {
    Status discover = DiscoverRawAssets();
    if (!discover) {
        return discover;
    }
    std::string errors;
    // Snapshot ids first; importers may add generated assets to the database.
    std::vector<AssetID> ids;
    for (AssetMetadata* metadata : m_Database.All()) {
        if (!metadata->rawPath.empty() && !IsGeneratedRawPath(metadata->rawPath) && !metadata->missing) {
            ids.push_back(metadata->id);
        }
    }
    for (const AssetID id : ids) {
        AssetMetadata* metadata = m_Database.Find(id);
        if (metadata == nullptr) {
            continue;
        }
        const Status status = ImportMetadata(*metadata);
        if (!status) {
            errors += status.error + "\n";
        }
    }
    m_Database.RebuildDependencyGraph();
    SaveDatabase();
    return errors.empty() ? Status::Ok() : Status::Fail(errors);
}

Status AssetManager::CookMetadata(AssetMetadata& metadata) {
    Cooker* cooker = FindCooker(metadata.type);
    if (cooker == nullptr) {
        return Status::Fail("no cooker registered for type " + AssetTypeToString(metadata.type));
    }

    CookContext context;
    context.metadata = metadata;
    context.rawRoot = m_Info.rawRoot;
    context.cookedRoot = m_Info.cookedRoot;
    context.projectRoot = m_Info.projectRoot;
    context.database = &m_Database;

    CookResult result = cooker->Cook(context);
    if (!result.success) {
        metadata.cooked = false;
        PushEvent({AssetEventType::Failed, metadata.id, metadata.rawPath, result.error});
        return Status::Fail(result.error);
    }

    metadata.cookedPath = ToProjectRelative(result.cookedPath);
    metadata.cooked = true;
    metadata.dirty = false;
    metadata.cookedTimestamp = NowSeconds();
    metadata.cookerVersion = cooker->Version();
    if (!result.dependencies.empty()) {
        metadata.dependencies = result.dependencies;
    }
    m_Database.AddOrUpdate(metadata);
    PushEvent({AssetEventType::Cooked, metadata.id, metadata.cookedPath, {}});
    return Status::Ok();
}

Status AssetManager::CookAsset(AssetID id) {
    AssetMetadata* metadata = m_Database.Find(id);
    if (metadata == nullptr) {
        return Status::Fail("unknown asset id: " + id.ToString());
    }
    const Status status = CookMetadata(*metadata);
    m_Database.RebuildDependencyGraph();
    SaveDatabase();
    return status;
}

Status AssetManager::CookAllDirty() {
    std::string errors;
    int guard = 0;
    bool progressed = true;
    while (progressed && guard++ < 64) {
        progressed = false;
        std::vector<AssetID> dirty;
        for (AssetMetadata* metadata : m_Database.DirtyAssets()) {
            if (FindCooker(metadata->type) == nullptr) {
                continue;
            }
            if (metadata->missing) {
                continue;
            }
            // A file-backed asset whose raw source has vanished (e.g. a texture
            // referenced by a material but never authored) cannot be cooked.
            // Mark it missing so dependents can report it and we don't spin.
            const bool fileBacked = !metadata->rawPath.empty() && !IsGeneratedRawPath(metadata->rawPath);
            if (fileBacked && !FileSystem::Exists(AbsoluteRaw(metadata->rawPath))) {
                metadata->missing = true;
                metadata->dirty = false;
                continue;
            }
            dirty.push_back(metadata->id);
        }
        for (const AssetID id : dirty) {
            AssetMetadata* metadata = m_Database.Find(id);
            if (metadata == nullptr || !metadata->dirty) {
                continue;
            }
            // Cook dependencies first: skip if any dependency is still dirty and
            // cookable this round.
            bool dependencyPending = false;
            for (const AssetID dep : metadata->dependencies) {
                const AssetMetadata* depMeta = m_Database.Find(dep);
                if (depMeta != nullptr && depMeta->dirty && FindCooker(depMeta->type) != nullptr) {
                    dependencyPending = true;
                    break;
                }
            }
            if (dependencyPending) {
                continue;
            }
            const Status status = CookMetadata(*metadata);
            if (!status) {
                errors += status.error + "\n";
                metadata->dirty = false; // avoid retrying a hard failure forever
            }
            progressed = true;
        }
    }
    m_Database.RebuildDependencyGraph();
    SaveDatabase();
    return errors.empty() ? Status::Ok() : Status::Fail(errors);
}

Status AssetManager::RecookAll() {
    for (AssetMetadata* metadata : m_Database.All()) {
        if (FindCooker(metadata->type) != nullptr) {
            metadata->dirty = true;
            metadata->cooked = false;
        }
    }
    return CookAllDirty();
}

Status AssetManager::SaveDatabase() {
    return m_Database.Save(m_Info.databasePath);
}

Status AssetManager::ValidateReferences(std::vector<AssetValidationIssue>* issues) {
    auto report = [&](const AssetMetadata& m, const std::string& msg, bool error) {
        if (issues != nullptr) {
            issues->push_back({m.id, m.rawPath, msg, error});
        }
        if (error) {
            HK_CORE_ERROR("[asset validate] {} ({}): {}", m.name, m.id.ToString(), msg);
        } else {
            HK_CORE_WARN("[asset validate] {} ({}): {}", m.name, m.id.ToString(), msg);
        }
    };

    bool anyError = false;
    for (const AssetMetadata* metadata : m_Database.All()) {
        // Raw file existence (skip generated sub-assets).
        if (!metadata->rawPath.empty() && !IsGeneratedRawPath(metadata->rawPath)) {
            if (!FileSystem::Exists(AbsoluteRaw(metadata->rawPath))) {
                report(*metadata, "raw file missing: " + metadata->rawPath.generic_string(), true);
                anyError = true;
            }
        }
        // Cooked file existence.
        if (metadata->cooked && !metadata->cookedPath.empty()) {
            if (!FileSystem::Exists(AbsoluteRaw(metadata->cookedPath))) {
                report(*metadata, "cooked file missing: " + metadata->cookedPath.generic_string(), true);
                anyError = true;
            }
        }
        // Stale cooked artifact.
        if (metadata->dirty && FindCooker(metadata->type) != nullptr) {
            report(*metadata, "asset is dirty (needs recook)", false);
        }
        // Dependency references.
        for (const AssetID dep : metadata->dependencies) {
            const AssetMetadata* depMeta = m_Database.Find(dep);
            if (depMeta == nullptr) {
                report(*metadata, "missing dependency AssetID " + dep.ToString(), true);
                anyError = true;
            } else if (depMeta->missing) {
                report(*metadata, "dependency raw file missing: " + dep.ToString(), true);
                anyError = true;
            }
        }
    }
    return anyError ? Status::Fail("asset validation found errors") : Status::Ok();
}

void AssetManager::StartWatching() {
    m_Watcher.Start(m_Info.rawRoot);
}
void AssetManager::StopWatching() {
    m_Watcher.Stop();
}

int AssetManager::PollHotReload(bool autoImport, bool autoCookDirty) {
    std::vector<fs::path> changed = m_Watcher.PollChangedFiles();
    if (changed.empty()) {
        return 0;
    }
    for (const fs::path& absolute : changed) {
        const AssetType type = ClassifyExtension(absolute);
        if (type == AssetType::Unknown) {
            continue;
        }
        const fs::path projectRel = ToProjectRelative(absolute);
        if (!FileSystem::Exists(absolute)) {
            if (AssetMetadata* metadata = m_Database.FindByRawPath(projectRel)) {
                metadata->missing = true;
                PushEvent({AssetEventType::Deleted, metadata->id, projectRel, {}});
            }
            continue;
        }
        if (autoImport) {
            ImportAsset(absolute);
            if (AssetMetadata* metadata = m_Database.FindByRawPath(projectRel)) {
                m_Database.MarkDirtyWithDependents(metadata->id);
                PushEvent({AssetEventType::Modified, metadata->id, projectRel, {}});
            }
        } else if (AssetMetadata* metadata = m_Database.FindByRawPath(projectRel)) {
            metadata->rawFileHash = AssetHash::HashFile(absolute);
            // A changed dependency must force its dependents to recook too.
            m_Database.MarkDirtyWithDependents(metadata->id);
            PushEvent({AssetEventType::Modified, metadata->id, projectRel, {}});
        }
    }
    if (autoCookDirty) {
        CookAllDirty();
        // A successful recook surfaces a reload event per cooked asset (already
        // emitted as Cooked); also emit Reloaded for clarity.
    }
    return static_cast<int>(changed.size());
}

const std::vector<AssetEvent>& AssetManager::PollEvents() {
    m_PolledEvents = std::move(m_Events);
    m_Events.clear();
    return m_PolledEvents;
}

void AssetManager::PushEvent(const AssetEvent& event) {
    m_Events.push_back(event);
}

AssetMetadata* AssetManager::EnsureCooked(AssetID id, Status& outStatus) {
    AssetMetadata* metadata = m_Database.Find(id);
    if (metadata == nullptr) {
        outStatus = Status::Fail("unknown asset id: " + id.ToString());
        return nullptr;
    }
    const fs::path cookedAbs = metadata->cookedPath.empty() ? fs::path{} : AbsoluteRaw(metadata->cookedPath);
    const bool cookedFileMissing = !cookedAbs.empty() && !FileSystem::Exists(cookedAbs);
    const bool needCook = !metadata->cooked || metadata->dirty || metadata->cookedPath.empty() || cookedFileMissing;
    if (needCook && FindCooker(metadata->type) != nullptr) {
        outStatus = CookMetadata(*metadata);
        if (!outStatus) {
            return nullptr;
        }
        SaveDatabase();
    }
    outStatus = Status::Ok();
    return metadata;
}

template <> Result<std::shared_ptr<TextureAsset>> AssetManager::Load<TextureAsset>(AssetID id) {
    if (auto cached = m_Registry.Get<TextureAsset>(id)) {
        return Result<std::shared_ptr<TextureAsset>>::Ok(std::move(cached));
    }
    Status status;
    AssetMetadata* metadata = EnsureCooked(id, status);
    if (metadata == nullptr) {
        return Result<std::shared_ptr<TextureAsset>>::Fail(status.error);
    }
    AssetLoader loader(m_Info.projectRoot);
    Result<std::shared_ptr<TextureAsset>> loaded = loader.LoadTexture(*metadata);
    if (!loaded) {
        return loaded;
    }
    m_Registry.Store<TextureAsset>(id, loaded.value);
    return loaded;
}

template <> Result<std::shared_ptr<MeshAsset>> AssetManager::Load<MeshAsset>(AssetID id) {
    if (auto cached = m_Registry.Get<MeshAsset>(id)) {
        return Result<std::shared_ptr<MeshAsset>>::Ok(std::move(cached));
    }
    Status status;
    AssetMetadata* metadata = EnsureCooked(id, status);
    if (metadata == nullptr) {
        return Result<std::shared_ptr<MeshAsset>>::Fail(status.error);
    }
    AssetLoader loader(m_Info.projectRoot);
    Result<std::shared_ptr<MeshAsset>> loaded = loader.LoadMesh(*metadata);
    if (!loaded) {
        return loaded;
    }
    m_Registry.Store<MeshAsset>(id, loaded.value);
    return loaded;
}

template <> Result<std::shared_ptr<ModelAsset>> AssetManager::Load<ModelAsset>(AssetID id) {
    if (auto cached = m_Registry.Get<ModelAsset>(id)) {
        return Result<std::shared_ptr<ModelAsset>>::Ok(std::move(cached));
    }
    Status status;
    AssetMetadata* metadata = EnsureCooked(id, status);
    if (metadata == nullptr) {
        return Result<std::shared_ptr<ModelAsset>>::Fail(status.error);
    }
    AssetLoader loader(m_Info.projectRoot);
    Result<std::shared_ptr<ModelAsset>> loaded = loader.LoadModel(*metadata);
    if (!loaded) {
        return loaded;
    }
    m_Registry.Store<ModelAsset>(id, loaded.value);
    return loaded;
}

template <> Result<std::shared_ptr<MaterialAsset>> AssetManager::Load<MaterialAsset>(AssetID id) {
    if (auto cached = m_Registry.Get<MaterialAsset>(id)) {
        return Result<std::shared_ptr<MaterialAsset>>::Ok(std::move(cached));
    }
    Status status;
    AssetMetadata* metadata = EnsureCooked(id, status);
    if (metadata == nullptr) {
        return Result<std::shared_ptr<MaterialAsset>>::Fail(status.error);
    }
    AssetLoader loader(m_Info.projectRoot);
    Result<std::shared_ptr<MaterialAsset>> loaded = loader.LoadMaterial(*metadata);
    if (!loaded) {
        return loaded;
    }
    m_Registry.Store<MaterialAsset>(id, loaded.value);
    return loaded;
}

template <> Result<std::shared_ptr<ShaderAsset>> AssetManager::Load<ShaderAsset>(AssetID id) {
    if (auto cached = m_Registry.Get<ShaderAsset>(id)) {
        return Result<std::shared_ptr<ShaderAsset>>::Ok(std::move(cached));
    }
    Status status;
    AssetMetadata* metadata = EnsureCooked(id, status);
    if (metadata == nullptr) {
        return Result<std::shared_ptr<ShaderAsset>>::Fail(status.error);
    }
    AssetLoader loader(m_Info.projectRoot);
    Result<std::shared_ptr<ShaderAsset>> loaded = loader.LoadShader(*metadata);
    if (!loaded) {
        return loaded;
    }
    m_Registry.Store<ShaderAsset>(id, loaded.value);
    return loaded;
}

void AssetManager::RegisterPipelines() {
    // Importers and cookers are registered here as each pipeline is implemented.
    RegisterImporter(std::make_unique<TextureImporter>());
    RegisterCooker(std::make_unique<TextureCooker>());
    RegisterImporter(std::make_unique<MaterialImporter>());
    RegisterCooker(std::make_unique<MaterialCooker>());
    RegisterImporter(std::make_unique<GltfImporter>());
    RegisterCooker(std::make_unique<MeshCooker>());
    RegisterCooker(std::make_unique<ModelCooker>());
    RegisterImporter(std::make_unique<ShaderImporter>());
    RegisterCooker(std::make_unique<ShaderCooker>());
    RegisterImporter(std::make_unique<SceneImporter>());
    RegisterCooker(std::make_unique<SceneCooker>());
    RegisterImporter(std::make_unique<PrefabImporter>());
    RegisterCooker(std::make_unique<PrefabCooker>());
}

} // namespace Hockey
