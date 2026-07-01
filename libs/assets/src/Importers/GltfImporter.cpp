#include "Hockey/Assets/Importers/GltfImporter.hpp"

#include "Gltf/GltfLoader.hpp"
#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Log.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <fstream>
#include <system_error>
#include <unordered_map>
#include <unordered_set>

#include <yaml-cpp/yaml.h>

namespace Hockey {
namespace fs = std::filesystem;

namespace {
void AddDependency(std::vector<AssetID>& deps, AssetID id) {
    if (id.IsValid() && std::find(deps.begin(), deps.end(), id) == deps.end()) {
        deps.push_back(id);
    }
}

std::string LowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string SanitizeStem(const std::string& value, const std::string& fallback) {
    std::string out;
    out.reserve(value.size());
    bool lastWasUnderscore = false;
    for (const unsigned char c : value) {
        const bool keep = std::isalnum(c) != 0 || c == '-' || c == '_';
        char next = keep ? static_cast<char>(c) : '_';
        if (next == '_') {
            if (lastWasUnderscore) {
                continue;
            }
            lastWasUnderscore = true;
        } else {
            lastWasUnderscore = false;
        }
        out.push_back(next);
    }
    while (!out.empty() && out.front() == '_') {
        out.erase(out.begin());
    }
    while (!out.empty() && out.back() == '_') {
        out.pop_back();
    }
    return out.empty() ? fallback : out;
}

std::string UniqueFilename(const std::string& stem, const std::string& extension,
                           std::unordered_set<std::string>& used) {
    const std::string cleanStem = SanitizeStem(stem, "asset");
    std::string candidate = cleanStem + extension;
    std::string key = LowerCopy(candidate);
    int suffix = 1;
    while (used.find(key) != used.end()) {
        candidate = cleanStem + "_" + std::to_string(suffix++) + extension;
        key = LowerCopy(candidate);
    }
    used.insert(key);
    return candidate;
}

fs::path ModelGroupPath(const fs::path& modelRawPath) {
    const fs::path modelsRoot = fs::path("data") / "raw" / "models";
    fs::path group = modelRawPath.lexically_relative(modelsRoot);
    const std::string groupString = group.generic_string();
    if (group.empty() || groupString == "." || groupString.rfind("..", 0) == 0) {
        group = modelRawPath.filename();
    }
    group.replace_extension();
    return group.empty() ? fs::path(modelRawPath.stem()) : group;
}

std::vector<fs::path> BuildGeneratedAssetPaths(const fs::path& modelRawPath, const std::vector<std::string>& names,
                                               const char* folder, const char* fallbackPrefix,
                                               const char* extension) {
    std::vector<fs::path> paths;
    paths.reserve(names.size());

    std::unordered_set<std::string> used;
    const fs::path group = ModelGroupPath(modelRawPath);
    for (size_t i = 0; i < names.size(); ++i) {
        const std::string fallback = std::string(fallbackPrefix) + "_" + std::to_string(i);
        const fs::path filename = UniqueFilename(names[i].empty() ? fallback : names[i], extension, used);
        paths.push_back((fs::path("data") / "raw" / folder / group / filename).generic_string());
    }
    return paths;
}

bool WriteBytesIfChanged(const fs::path& outPath, const std::vector<std::byte>& bytes) {
    std::error_code ec;
    fs::create_directories(outPath.parent_path(), ec);
    if (ec) {
        HK_CORE_WARN("GltfImporter: failed to create directory '{}': {}", outPath.parent_path().string(),
                     ec.message());
        return false;
    }

    if (fs::exists(outPath, ec)) {
        const auto existingSize = fs::file_size(outPath, ec);
        if (!ec && existingSize == bytes.size()) {
            return true;
        }
    }

    std::ofstream out(outPath, std::ios::binary | std::ios::trunc);
    if (!out) {
        HK_CORE_WARN("GltfImporter: failed to write embedded texture {}", outPath.string());
        return false;
    }
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(out);
}

bool CopyTextureIfChanged(const fs::path& sourcePath, const fs::path& outPath) {
    std::error_code ec;
    if (fs::equivalent(sourcePath, outPath, ec)) {
        return true;
    }
    ec.clear();
    fs::create_directories(outPath.parent_path(), ec);
    if (ec) {
        HK_CORE_WARN("GltfImporter: failed to create directory '{}': {}", outPath.parent_path().string(),
                     ec.message());
        return false;
    }

    if (fs::exists(sourcePath, ec)) {
        const uintmax_t sourceSize = fs::file_size(sourcePath, ec);
        std::error_code outEc;
        if (!ec && fs::exists(outPath, outEc) && fs::file_size(outPath, outEc) == sourceSize) {
            return true;
        }
    }

    fs::copy_file(sourcePath, outPath, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        HK_CORE_WARN("GltfImporter: failed to copy texture '{}' -> '{}': {}", sourcePath.string(), outPath.string(),
                     ec.message());
        return false;
    }
    return true;
}

void RegisterGeneratedFile(AssetDatabase* db, std::vector<AssetMetadata>& out, const fs::path& rawPath,
                           AssetType type, const std::string& name, const std::string& importerVersion) {
    AssetMetadata meta;
    meta.type = type;
    meta.rawPath = rawPath.generic_string();
    meta.metadataPath = AssetPath::MetadataSidecar(meta.rawPath);
    meta.name = name.empty() ? rawPath.stem().string() : name;
    meta.importerVersion = importerVersion;
    if (db != nullptr) {
        meta.id = db->GetOrCreateIDForPath(meta.rawPath, type);
    }
    out.push_back(std::move(meta));
}

MaterialSource SourceFromGltf(const GltfMaterialData& gltf,
                              const std::function<fs::path(const std::string&)>& texturePath) {
    MaterialSource source;
    source.name = gltf.name;
    source.baseColor = gltf.baseColor;
    source.metallic = gltf.metallic;
    source.roughness = gltf.roughness;
    source.normalStrength = gltf.normalStrength;
    source.occlusionStrength = gltf.occlusionStrength;
    source.emissiveColor = gltf.emissiveColor;
    source.emissiveStrength = gltf.emissiveStrength;
    source.alphaMode = gltf.alphaMode;
    source.alphaCutoff = gltf.alphaCutoff;

    auto toString = [&](const std::string& uri) -> std::string {
        const fs::path path = texturePath(uri);
        return path.empty() ? std::string{} : path.generic_string();
    };
    source.baseColorTexture = toString(gltf.baseColorTexture);
    source.normalTexture = toString(gltf.normalTexture);
    source.metallicRoughnessTexture = toString(gltf.metallicRoughnessTexture);
    source.occlusionTexture = toString(gltf.occlusionTexture);
    source.emissiveTexture = toString(gltf.emissiveTexture);
    return source;
}

Status WriteMeshDescriptor(const fs::path& path, const fs::path& modelRawPath, size_t meshIndex,
                           const std::string& meshName, const std::vector<fs::path>& materialPaths,
                           const fs::path& skeletonPath) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Mesh" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "SourceModel" << YAML::Value << modelRawPath.generic_string();
    out << YAML::Key << "MeshIndex" << YAML::Value << static_cast<uint64_t>(meshIndex);
    out << YAML::Key << "Name" << YAML::Value << meshName;
    out << YAML::Key << "Materials" << YAML::Value << YAML::BeginSeq;
    for (const fs::path& materialPath : materialPaths) {
        out << materialPath.generic_string();
    }
    out << YAML::EndSeq;
    if (!skeletonPath.empty()) {
        out << YAML::Key << "Skeleton" << YAML::Value << skeletonPath.generic_string();
    }
    out << YAML::EndMap;
    out << YAML::EndMap;

    return FileSystem::WriteTextFile(path, out.c_str());
}

Status WriteSkeletonDescriptor(const fs::path& path, const fs::path& modelRawPath, size_t skinIndex,
                               const std::string& skeletonName) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Skeleton" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "SourceModel" << YAML::Value << modelRawPath.generic_string();
    out << YAML::Key << "SkinIndex" << YAML::Value << static_cast<uint64_t>(skinIndex);
    out << YAML::Key << "Name" << YAML::Value << skeletonName;
    out << YAML::EndMap;
    out << YAML::EndMap;

    return FileSystem::WriteTextFile(path, out.c_str());
}

Status WriteAnimationDescriptor(const fs::path& path, const fs::path& modelRawPath, size_t animationIndex,
                                const std::string& animationName, const fs::path& skeletonPath) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Animation" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "SourceModel" << YAML::Value << modelRawPath.generic_string();
    out << YAML::Key << "AnimationIndex" << YAML::Value << static_cast<uint64_t>(animationIndex);
    out << YAML::Key << "Name" << YAML::Value << animationName;
    out << YAML::Key << "Skeleton" << YAML::Value << skeletonPath.generic_string();
    out << YAML::EndMap;
    out << YAML::EndMap;

    return FileSystem::WriteTextFile(path, out.c_str());
}
} // namespace

bool GltfImporter::SupportsExtension(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".gltf" || ext == ".glb";
}

std::string GltfImporter::MeshSubAssetPath(const fs::path& modelRawPath, size_t meshIndex) {
    return modelRawPath.generic_string() + "#mesh" + std::to_string(meshIndex);
}

std::string GltfImporter::MaterialSubAssetPath(const fs::path& modelRawPath, size_t materialIndex) {
    return modelRawPath.generic_string() + "#material" + std::to_string(materialIndex);
}

std::vector<fs::path> GltfImporter::MeshAssetPaths(const fs::path& modelRawPath,
                                                   const std::vector<std::string>& meshNames) {
    return BuildGeneratedAssetPaths(modelRawPath, meshNames, "meshes", "mesh", ".mesh.yaml");
}

std::vector<fs::path> GltfImporter::MaterialAssetPaths(const fs::path& modelRawPath,
                                                       const std::vector<std::string>& materialNames) {
    return BuildGeneratedAssetPaths(modelRawPath, materialNames, "materials", "material", ".material.yaml");
}

std::vector<fs::path> GltfImporter::SkeletonAssetPaths(const fs::path& modelRawPath,
                                                       const std::vector<std::string>& skeletonNames) {
    return BuildGeneratedAssetPaths(modelRawPath, skeletonNames, "animation/skeletons", "skeleton", ".skeleton.yaml");
}

std::vector<fs::path> GltfImporter::AnimationAssetPaths(const fs::path& modelRawPath,
                                                        const std::vector<std::string>& animationNames) {
    return BuildGeneratedAssetPaths(modelRawPath, animationNames, "animation/clips", "animation", ".anim.yaml");
}

bool GltfImporter::IsSubAsset(const fs::path& rawPath) {
    return rawPath.generic_string().find('#') != std::string::npos;
}

bool GltfImporter::ParseSubAsset(const fs::path& rawPath, fs::path& outModelRawPath, size_t& outIndex,
                                 bool& outIsMesh) {
    const std::string s = rawPath.generic_string();
    const size_t hash = s.find('#');
    if (hash == std::string::npos) {
        return false;
    }
    outModelRawPath = s.substr(0, hash);
    const std::string token = s.substr(hash + 1);

    std::string prefix;
    if (token.rfind("mesh", 0) == 0) {
        outIsMesh = true;
        prefix = "mesh";
    } else if (token.rfind("material", 0) == 0) {
        outIsMesh = false;
        prefix = "material";
    } else {
        return false;
    }
    try {
        outIndex = static_cast<size_t>(std::stoul(token.substr(prefix.size())));
    } catch (...) {
        return false;
    }
    return true;
}

fs::path GltfImporter::TextureProjectPath(const fs::path& projectRoot, const fs::path& modelRawPath,
                                          const std::string& uri) {
    if (uri.empty()) {
        return {};
    }
    const fs::path modelAbsolute = projectRoot / modelRawPath;
    const fs::path textureAbsolute = modelAbsolute.parent_path() / uri;
    return AssetPath::ToProjectRelative(projectRoot, textureAbsolute);
}

ImportResult GltfImporter::Import(const ImportContext& context) {
    ImportResult result;

    Result<GltfScene> scene = GltfLoader::Load(context.rawPath);
    if (!scene) {
        result.success = false;
        result.error = scene.error;
        return result;
    }

    const fs::path modelRawPath = AssetPath::ToProjectRelative(context.projectRoot, context.rawPath);

    result.success = true;
    result.metadata.id = context.existingId;
    result.metadata.type = AssetType::Model;
    result.metadata.name = context.rawPath.stem().string();

    AssetDatabase* db = context.database;
    std::vector<AssetID> modelDeps;

    std::unordered_map<std::string, const GltfEmbeddedTexture*> embeddedByPath;
    for (const GltfEmbeddedTexture& texture : scene.value.embeddedTextures) {
        embeddedByPath.emplace(texture.relativePath, &texture);
    }

    std::unordered_map<std::string, fs::path> texturePathByUri;
    std::unordered_set<std::string> usedTextureNames;
    std::unordered_set<std::string> registeredTexturePaths;
    const fs::path textureGroup = ModelGroupPath(modelRawPath);
    auto textureProjectPath = [&](const std::string& uri) -> fs::path {
        if (uri.empty()) {
            return {};
        }
        if (const auto found = texturePathByUri.find(uri); found != texturePathByUri.end()) {
            return found->second;
        }

        const fs::path uriPath(uri);
        std::string extension = uriPath.extension().generic_string();
        std::string stem = uriPath.stem().string();
        if (extension.empty()) {
            extension = ".bin";
        }
        const fs::path filename = UniqueFilename(stem.empty() ? "texture" : stem, extension, usedTextureNames);
        const fs::path rel = (fs::path("data") / "raw" / "textures" / textureGroup / filename).generic_string();
        const fs::path outAbs = context.projectRoot / rel;

        if (const auto embedded = embeddedByPath.find(uri); embedded != embeddedByPath.end()) {
            WriteBytesIfChanged(outAbs, embedded->second->bytes);
        } else {
            CopyTextureIfChanged(context.rawPath.parent_path() / uriPath, outAbs);
        }

        texturePathByUri.emplace(uri, rel);
        if (registeredTexturePaths.insert(rel.generic_string()).second) {
            RegisterGeneratedFile(db, result.generatedAssets, rel, AssetType::Texture, rel.stem().string(),
                                  Version());
        }
        return rel;
    };

    std::vector<std::string> materialNames;
    materialNames.reserve(scene.value.materials.size());
    for (const GltfMaterialData& material : scene.value.materials) {
        materialNames.push_back(material.name);
    }
    const std::vector<fs::path> materialPaths = MaterialAssetPaths(modelRawPath, materialNames);

    std::vector<std::string> meshNames;
    meshNames.reserve(scene.value.meshes.size());
    for (const GltfMeshData& mesh : scene.value.meshes) {
        meshNames.push_back(mesh.name);
    }
    const std::vector<fs::path> meshPaths = MeshAssetPaths(modelRawPath, meshNames);

    std::vector<std::string> skeletonNames;
    skeletonNames.reserve(scene.value.skeletons.size());
    for (const GltfSkeletonData& skeleton : scene.value.skeletons) {
        skeletonNames.push_back(skeleton.name);
    }
    const std::vector<fs::path> skeletonPaths = SkeletonAssetPaths(modelRawPath, skeletonNames);

    std::vector<std::string> animationNames;
    animationNames.reserve(scene.value.animations.size());
    for (const GltfAnimationData& animation : scene.value.animations) {
        animationNames.push_back(animation.name);
    }
    const std::vector<fs::path> animationPaths = AnimationAssetPaths(modelRawPath, animationNames);

    // Generated material sub-assets (and their texture dependencies).
    std::vector<AssetID> materialIds(scene.value.materials.size());
    for (size_t i = 0; i < scene.value.materials.size(); ++i) {
        const GltfMaterialData& material = scene.value.materials[i];
        const fs::path subPath = materialPaths[i];
        const MaterialSource source = SourceFromGltf(material, textureProjectPath);
        const Status wrote = MaterialSerializer::SaveFile(context.projectRoot / subPath, source);
        if (!wrote) {
            result.success = false;
            result.error = wrote.error;
            return result;
        }

        AssetMetadata meta;
        meta.type = AssetType::Material;
        meta.rawPath = subPath.generic_string();
        meta.metadataPath = AssetPath::MetadataSidecar(meta.rawPath);
        meta.name = material.name;
        meta.importerVersion = Version();

        if (db != nullptr) {
            meta.id = db->GetOrCreateIDForPath(subPath, AssetType::Material);
            for (const std::string& texturePath : MaterialSerializer::TexturePaths(source)) {
                AddDependency(meta.dependencies, db->GetOrCreateIDForPath(texturePath, AssetType::Texture));
            }
        }
        materialIds[i] = meta.id;
        AddDependency(modelDeps, meta.id);
        result.generatedAssets.push_back(std::move(meta));
    }

    // Generated skeleton assets.
    std::vector<AssetID> skeletonIds(scene.value.skeletons.size());
    for (size_t i = 0; i < scene.value.skeletons.size(); ++i) {
        const GltfSkeletonData& skeleton = scene.value.skeletons[i];
        const fs::path subPath = skeletonPaths[i];
        const Status wrote = WriteSkeletonDescriptor(context.projectRoot / subPath, modelRawPath, i, skeleton.name);
        if (!wrote) {
            result.success = false;
            result.error = wrote.error;
            return result;
        }

        AssetMetadata meta;
        meta.type = AssetType::Skeleton;
        meta.rawPath = subPath.generic_string();
        meta.metadataPath = AssetPath::MetadataSidecar(meta.rawPath);
        meta.name = skeleton.name;
        meta.importerVersion = Version();

        if (db != nullptr) {
            meta.id = db->GetOrCreateIDForPath(subPath, AssetType::Skeleton);
        }
        skeletonIds[i] = meta.id;
        AddDependency(modelDeps, meta.id);
        result.generatedAssets.push_back(std::move(meta));
    }

    // Generated mesh sub-assets (depend on the materials their submeshes use).
    for (size_t i = 0; i < scene.value.meshes.size(); ++i) {
        const GltfMeshData& mesh = scene.value.meshes[i];
        const fs::path subPath = meshPaths[i];
        fs::path skeletonPath;
        if (mesh.skinIndex >= 0 && static_cast<size_t>(mesh.skinIndex) < skeletonPaths.size()) {
            skeletonPath = skeletonPaths[static_cast<size_t>(mesh.skinIndex)];
        }
        const Status wrote =
            WriteMeshDescriptor(context.projectRoot / subPath, modelRawPath, i, mesh.name, materialPaths, skeletonPath);
        if (!wrote) {
            result.success = false;
            result.error = wrote.error;
            return result;
        }

        AssetMetadata meta;
        meta.type = AssetType::Mesh;
        meta.rawPath = subPath.generic_string();
        meta.metadataPath = AssetPath::MetadataSidecar(meta.rawPath);
        meta.name = mesh.name;
        meta.importerVersion = Version();

        if (db != nullptr) {
            meta.id = db->GetOrCreateIDForPath(subPath, AssetType::Mesh);
            for (const int materialIndex : mesh.submeshMaterialIndex) {
                if (materialIndex >= 0 && static_cast<size_t>(materialIndex) < materialIds.size()) {
                    AddDependency(meta.dependencies, materialIds[materialIndex]);
                }
            }
            if (mesh.skinIndex >= 0 && static_cast<size_t>(mesh.skinIndex) < skeletonIds.size()) {
                AddDependency(meta.dependencies, skeletonIds[static_cast<size_t>(mesh.skinIndex)]);
            }
        }
        AddDependency(modelDeps, meta.id);
        result.generatedAssets.push_back(std::move(meta));
    }

    // Generated animation clip assets.
    for (size_t i = 0; i < scene.value.animations.size(); ++i) {
        const GltfAnimationData& animation = scene.value.animations[i];
        if (animation.skeletonIndex < 0 || static_cast<size_t>(animation.skeletonIndex) >= skeletonPaths.size()) {
            continue;
        }

        const fs::path subPath = animationPaths[i];
        const fs::path skeletonPath = skeletonPaths[static_cast<size_t>(animation.skeletonIndex)];
        const Status wrote =
            WriteAnimationDescriptor(context.projectRoot / subPath, modelRawPath, i, animation.name, skeletonPath);
        if (!wrote) {
            result.success = false;
            result.error = wrote.error;
            return result;
        }

        AssetMetadata meta;
        meta.type = AssetType::Animation;
        meta.rawPath = subPath.generic_string();
        meta.metadataPath = AssetPath::MetadataSidecar(meta.rawPath);
        meta.name = animation.name;
        meta.importerVersion = Version();

        if (db != nullptr) {
            meta.id = db->GetOrCreateIDForPath(subPath, AssetType::Animation);
            AddDependency(meta.dependencies, skeletonIds[static_cast<size_t>(animation.skeletonIndex)]);
        }
        AddDependency(modelDeps, meta.id);
        result.generatedAssets.push_back(std::move(meta));
    }

    result.metadata.dependencies = modelDeps;
    return result;
}

} // namespace Hockey
